#pragma once
#include <array>
#include <algorithm>
#include <numeric>
#include <random>
#include "vec3.h"

// Genera rumore deterministico e continuo: stesso punto -> stesso valore, punti vicini -> valori simili
class Perlin
{
	std::array<int, 256> perm_x; // permutazione di 0..255 usata per l'asse x
	std::array<int, 256> perm_y; // permutazione di 0..255 usata per l'asse y
	std::array<int, 256> perm_z; // permutazione di 0..255 usata per l'asse z
	std::array<double, 256> rand_double; // valori scalari casuali (usati da noise/noise_smooth)
	std::array<Vec3, 256> rand_vec;      // gradienti casuali unitari (usati da noise_smooth_gradient)

public:
	Perlin()
		: perm_x(generate_permutation()),  perm_y(generate_permutation()), perm_z(generate_permutation()), rand_double(generate_random_doubles()), rand_vec(generate_random_vec())
	{ }

	// Riempie 0..255 in ordine, poi mescola: una permutazione casuale di 256 interi
	static std::array<int, 256> generate_permutation() 
	{
		std::array<int, 256> p;
		std::iota(p.begin(), p.end(), 0);
		std::random_device rd;
		std::mt19937 gen(rd());
		std::shuffle(p.begin(), p.end(), gen);
		return p;
	}

	// 256 double casuali indipendenti in [0,1), usati come valori "grezzi" del rumore
	static std::array<double, 256> generate_random_doubles() 
	{
		std::array<double, 256> p;
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<double> dist(0.0, 1.0);
		for (int i = 0; i < p.size(); i++)
		{
			p[i] = dist(gen);
		}
		return p;
	}

	// 256 vettori unitari casuali, usati come gradienti (Perlin noise "vero")
	static std::array<Vec3, 256> generate_random_vec() 
	{
		std::array<Vec3, 256> p;
		for (int i = 0; i < p.size(); i++)
		{
			p[i] = random_unit_vector();
		}
		return p;
	}

	// Rumore "a blocchetti": un solo lookup per cella, nessuna interpolazione
	double noise(const Point3& p) const
	{
		auto x = static_cast<int>(floor(p.x) );
		auto y = static_cast<int>( floor(p.y) );
		auto z = static_cast<int>( floor(p.z) );
		auto i = x & 255; // riduce l'indice al range valido 0-255
		auto j = y & 255;
		auto k = z & 255;
		auto perm_value_x = perm_x[i];
		auto perm_value_y = perm_y[j];
		auto perm_value_z = perm_z[k];
		auto index = perm_value_x ^ perm_value_y ^ perm_value_z; // combina i tre assi in un unico indice
		return rand_double[index];
	}

	// Rumore continuo: interpolazione trilineare sugli 8 valori scalari agli angoli della cella
	double noise_smooth(const Point3& p) const
	{
		auto x = static_cast<int>(floor(p.x) );
		auto y = static_cast<int>( floor(p.y) );
		auto z = static_cast<int>( floor(p.z) );
		auto u = p.x - x; // posizione frazionaria di p dentro la cella
		auto v = p.y - y;
		auto w = p.z - z;
		auto i = x & 255;
		auto j = y & 255;
		auto k = z & 255;
		double accum = 0.0;
		for (int di = 0; di < 2; di++) // 8 angoli del cubo unitario = combinazioni di di,dj,dk in {0,1}
		{
			for (int dj = 0; dj < 2; dj++)
			{
				for (int dk = 0; dk < 2; dk++)
				{
					auto u_weight = di ? u : 1 - u; // peso trilineare per questo angolo
					auto v_weight = dj ? v : 1 - v;
					auto w_weight = dk ? w : 1 - w;
					auto perm_value_x = perm_x[(i+di)&255]; // &255 evita overflow quando i/j/k=255
					auto perm_value_y = perm_y[(j+dj)&255];
					auto perm_value_z = perm_z[(k+dk)&255];
					auto index = perm_value_x ^ perm_value_y ^ perm_value_z;
					auto value = rand_double[index];
					accum += u_weight * v_weight * w_weight * value;
				}
			}
		}
		return accum;	 
	}

	// Vero Perlin noise: gradienti + dot product + Hermitian smoothing, elimina il Mach banding
	double noise_smooth_gradient(const Point3& p) const
	{
		auto x = static_cast<int>(floor(p.x) );
		auto y = static_cast<int>( floor(p.y) );
		auto z = static_cast<int>( floor(p.z) );
		auto u = p.x - x; // posizione frazionaria grezza, usata nel vettore di spostamento
		auto v = p.y - y;
		auto w = p.z - z;
		auto u_smooth = u*u*(3-2*u); // smoothstep: derivata nulla agli estremi, elimina cambi bruschi di pendenza
		auto v_smooth = v*v*(3-2*v);
		auto w_smooth = w*w*(3-2*w);
		auto i = x & 255;
		auto j = y & 255;
		auto k = z & 255;
		double accum = 0.0;
		for (int di = 0; di < 2; di++)
		{
			for (int dj = 0; dj < 2; dj++)
			{
				for (int dk = 0; dk < 2; dk++)
				{
					auto u_weight = di ? u_smooth : 1 - u_smooth; // pesi calcolati sulla versione smussata
					auto v_weight = dj ? v_smooth : 1 - v_smooth;
					auto w_weight = dk ? w_smooth : 1 - w_smooth;
					auto perm_value_x = perm_x[(i+di)&255];
					auto perm_value_y = perm_y[(j+dj)&255];
					auto perm_value_z = perm_z[(k+dk)&255];
					auto index = perm_value_x ^ perm_value_y ^ perm_value_z;
					auto value = rand_vec[index]; // gradiente casuale dell'angolo
					// contributo = dot(gradiente, spostamento angolo->p), spostamento con u/v/w grezzi
					accum += u_weight*v_weight*w_weight*dot(value, { u - di, v - dj, w - dk });
				}
			}
		}
		return accum;	 
	}

	// Somma di più ottave di rumore a frequenza crescente e peso decrescente, per dettaglio "naturale"
	double turbolence(const Point3& p, int depth) const
	{
		double sum_noise = 0;
		auto p_copy = p;
		auto weight = 1.0;
		for(int i = 0; i<depth; i++)
		{
			sum_noise += weight*noise_smooth_gradient(p_copy); // accumula col peso corrente
			weight *= 0.5;   // ogni ottava successiva conta la metà della precedente
			p_copy *= 2;     // ogni ottava successiva ha il doppio della frequenza
		}
		return std::fabs(sum_noise); // il risultato può essere negativo: riportato sempre positivo
	}
};

/*
* RIEPILOGO DEL FILE
*
* Perlin genera rumore pseudo-casuale ma deterministico e continuo: dato lo
* stesso punto p, restituisce sempre lo stesso valore; punti vicini
* restituiscono valori simili (a differenza di un rand() puro per punto,
* che darebbe "sale e pepe" senza continuità).
*
* Il meccanismo di base (permutation table + hashing): tre tabelle di
* permutazione (perm_x/y/z), generate una sola volta nel costruttore,
* mappano le coordinate intere di p (ridotte con &255) su indici 0-255.
* I tre indici ottenuti vengono combinati con XOR in un unico indice
* finale, usato per pescare un valore "vero" (double o Vec3) da un'ultima
* tabella. Questo evita di dover tenere una tabella 3D gigante (256^3),
* usando solo 3 tabelle 1D da 256 elementi.
*
* noise(): versione base, un solo lookup per cella -> rumore "a blocchetti"
* (nessuna continuità tra celle adiacenti).
*
* noise_smooth(): interpolazione trilineare sugli 8 angoli del cubo che
* contiene p, usando valori scalari (rand_double). Continuo, ma soffre di
* Mach banding (l'occhio percepisce i confini delle celle come deboli bande,
* perché la derivata del rumore non è continua alle pareti tra celle).
*
* noise_smooth_gradient(): il vero algoritmo di Perlin. Agli angoli non si
* mette un valore scalare ma un gradiente (vettore unitario casuale,
* rand_vec); il contributo di un angolo è il dot product tra il suo
* gradiente e il vettore che va dall'angolo a p. In più, i pesi
* dell'interpolazione usano u/v/w passati per una funzione smoothstep
* (u*u*(3-2*u)), che smussa ulteriormente la transizione tra celle
* eliminando il Mach banding residuo. Il vettore di spostamento nel dot
* product usa invece u/v/w grezzi (non smussati), per restare geometricamente
* corretto. Il risultato è in circa [-1,1], non [0,1].
*
* turbolence(): somma "depth" chiamate a noise_smooth_gradient, raddoppiando
* il punto (quindi la frequenza) e dimezzando il peso ad ogni iterazione.
* Il risultato è una texture con dettaglio a più scale sovrapposte (macchie
* larghe + grana fine), invece di una singola scala di rumore. fabs() finale
* perché la somma di termini con segno può risultare negativa.
*
* NoiseTexture (in texture.h) usa queste funzioni per produrre tre effetti:
* rumore liscio diretto, turbolenza diretta ("rete mimetica"), o turbolenza
* usata per modulare la fase di un seno (venature di marmo).
*/