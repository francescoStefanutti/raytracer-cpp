#pragma once



/* Perchè non abbiamo solo declaration in questo header ma proprio la definition anche delle funzioni?

Classi complesse (Es: Nemico, Renderer, SistemaAudio): Dichiarazione nel .h, definizione nel .cpp.

Classi matematiche minuscole (Es: Vec3, matrici, quaternioni): Tutto nel .h per permettere l'inlining e spremere al massimo la CPU.*/

#include <cmath> // (per calcolare radici quadrate e lunghezze dei vettori).
#include <iostream>
#include "essentials.h"

struct Vec3
{
	double x{ 0.0 }, y{ 0.0 }, z{ 0.0 }; // le parentesi graffe indicano il valore di default

	/*Vec3()
		:x(0), y(0), z(0)
	{ }*/

	constexpr Vec3() = default; // Il compilatore userà gli zeri scritti sopra. Sostituisce il costruttore che si usava sopra nel c++ più vecchio

	constexpr Vec3(double x, double y, double z)
		:x(x), y(y), z(z)
	{
	}

	constexpr Vec3 operator-() const
	{
		return Vec3(-x, -y, -z);
	}

	constexpr Vec3& operator+=(const Vec3& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;

		return *this;
	}

	constexpr Vec3& operator*=(double t)
	{
		x *= t;
		y *= t;
		z *= t;

		return *this;
	}

	constexpr Vec3& operator/=(double t)
	{
		// sfrutto l'operatore moltiplicazione che ho appena creato
		// ricorda che moltiplicazione è meno costosa che divisione
		return *this *= (1.0 / t);
	}

	//: le radici quadrate (std::sqrt) sono operazioni lentissime per la CPU.
	// Per questo motivo, scriveremo due funzioni separate. Quando dovremo solo confrontare due distanze
	// (per sapere quale oggetto è più vicino), useremo la lunghezza "al quadrato".
	// Quando ci servirà la lunghezza vera e propria, useremo quella con la radice.

	constexpr double length_squared() const
	{
		return x * x + y * y + z * z;
	}

	[[nodiscard]] double length() const
	{
		return std::sqrt(length_squared());
	}

	// metodo per controllare che tutte le componenti del vettore non siano nulle. ci serve per il calcolo del colore per controllare che il vettore riflessione non sia nullo
	[[nodiscard]] bool near_zero() const
	{
		return std::fabs(x) < 1e-8 && std::fabs(y) < 1e-8 && std::fabs(z) < 1e-8; //std::fabs si usa per calcolare valore assoluto
	}
};

inline constexpr Vec3 operator+(const Vec3& u, const Vec3& v)
{
	return Vec3(u.x + v.x, u.y + v.y, u.z + v.z);
}

inline constexpr Vec3 operator-(const Vec3& u, const Vec3& v)
{
	return Vec3(u.x - v.x, u.y - v.y, u.z - v.z);
}

inline constexpr Vec3 operator*(const Vec3& u, const double t)
{
	return Vec3(u.x*t, u.y*t, u.z*t);
}

inline constexpr Vec3 operator*(const double t, const Vec3& u)
{
	// uso funzione precedente
	return u*t;
}

inline constexpr Vec3 operator*(const Vec3& u, const Vec3& v)
{
	return Vec3(u.x * v.x, u.y * v.y, u.z * v.z);
}

inline constexpr Vec3 operator/(const Vec3& u, const double t)
{
	return u*(1.0/t);
}

inline constexpr double dot(const Vec3& u, const Vec3& v)
{
	return u.x * v.x + u.y * v.y + u.z * v.z;
}

inline constexpr Vec3 cross(const Vec3& u, const Vec3& v)
{
	return Vec3(u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x);
}

[[nodiscard]] inline Vec3 unit_vector(Vec3 v)
{
	return v / v.length();
}

inline Vec3 random_vec3() 
{
	return Vec3(random_double(), random_double(), random_double());
}

inline Vec3 random_vec3(double min, double max)
{
	return Vec3{ random_double(min,max), random_double(min,max), random_double(min,max) };
}


inline Vec3 random_unit_vector()
{
	while (true)
	{
		auto p = random_vec3(-1.0, 1.0);

		auto lensq = p.length_squared();

		if (lensq > 1e-160 && lensq <= 1.0)
			return p / std::sqrt(lensq);
	}
}

inline Vec3 random_in_unit_disk()
{
	while (true)
	{
		auto p = Vec3(random_double(-1.0, 1.0), random_double(-1.0, 1.0), 0);

		auto lensq = p.length_squared();

		if (lensq <= 1.0)
			return p;
	}
}

inline constexpr Vec3 reflect(const Vec3& v, const Vec3& n)
{
	return v - 2 * dot(v, n) * n;
}

inline Vec3 refract(const Vec3& v, const Vec3& n, double etai_over_etat) // dove `etai_over_etat` è il rapporto `n1/n2` che il chiamante (`Dielectric::scatter()`) calcolerà e passerà già pronto.
{
	auto cos_theta = std::fmin(dot(-v, n), 1.0); //fmin serve affinchè non vada oltre ad 1
	
	auto r_out_perp = etai_over_etat * (v + cos_theta * n);
	auto r_out_paral = -std::sqrt(std::fabs(1.0 - r_out_perp.length_squared())) * n; // std::fabs()` per sicurezza numerica, evitando che un numero leggermente negativo (per errori di arrotondamento) faccia fallire la `sqrt`.
	
	return r_out_paral + r_out_perp;
}



using Point3 = Vec3;
using Color = Vec3;

/*
* ==========================================
* APPUNTI DA SENIOR DEV: INLINE E OPERATORI ESTERNI
* ==========================================
* 
* 1. Perché usare "inline" per le funzioni libere nel file .h:
*    - #pragma once previene l'inclusione multipla in un SINGOLO file .cpp.
*    - Ma se più file .cpp (es. main.cpp, sfera.cpp) includono questo header, 
*      il Linker troverà copie multiple delle stesse funzioni esterne e andrà in crash 
*      (Violazione della ODR - One Definition Rule).
*    - La parola chiave "inline" avvisa il Linker di unificare queste copie senza dare errori.
* 
* 2. Inline Implicito vs Esplicito:
*    - Le funzioni definite DENTRO la struct (es. operator+=) sono già "inline" di default.
*    - Le funzioni definite FUORI dalla struct (nel file .h) richiedono la parola "inline" esplicita.
* 
* 3. Perché scrivere operatori (+, -, *) FUORI dalla struct:
*    - "Dittatura del lato sinistro": gli operatori interni richiedono che l'oggetto 
*      proprietario sia sempre a sinistra dell'equazione (es. "vettore * 2.0").
*    - Per scrivere "2.0 * vettore" dovremmo aggiungere un metodo al tipo primitivo 
*      'double', cosa impossibile in C++.
*    - Usando funzioni globali (esterne) possiamo definire l'ordine esatto degli 
*      argomenti (es. (double, Vec3) oppure (Vec3, double)).
*    - Convenzione C++: gli operatori binari simmetrici che generano un NUOVO oggetto 
*      vanno messi fuori dalla struct per mantenere il codice flessibile ed elegante.
* ==========================================
*/

/*
* ==========================================
* DIFFERENZA TRA "*=" e "*"
* ==========================================
* - operator*= (Modifica in-place): 
*   Altera il vettore originale e restituisce una reference (*this). 
*   Non alloca nuova memoria. Utile per aggiornare uno stato.
* 
* - operator* (Creazione out-of-place): 
*   Genera e restituisce un vettore NUOVO di zecca (return Vec3(...)). 
*   L'originale rimane intatto. Indispensabile per le equazioni (es. A = B * C).
* ==========================================
*/


/*
* ==========================================
* APPUNTI DA SENIOR DEV: CONSTEXPR E NODISCARD
* ==========================================
* 
* 1. La Magia di "constexpr" (Constant Expression):
*    - Suggerisce al compilatore di eseguire i calcoli DURANTE la compilazione, 
*      invece che a runtime (mentre il programma gira).
*    - Se scrivi: Vec3(1,0,0) + Vec3(0,1,0), il compilatore non farà l'addizione
*      nel gioco finito. Inserirà direttamente il risultato Vec3(1,1,0) nell'exe!
*    - Vantaggio: Costo zero per la CPU a runtime. Ottimizzazione estrema.
* 
* 2. Regole di posizionamento:
*    - Costruttori: constexpr Vec3()
*    - Metodi interni: constexpr Vec3 operator-() const
*    - Funzioni esterne: inline constexpr Vec3 operator+(...)
* 
* 3. L'eccezione std::sqrt:
*    - Le radici quadrate (std::sqrt) storicamente non sono calcolabili a compile-time
*      negli standard C++ più vecchi.
*    - Per questo motivo, "length()" e "unit_vector()" NON hanno constexpr.
* 
* 4. Sicurezza con [[nodiscard]] (C++17):
*    - Dice al compilatore: "Se qualcuno chiama questa funzione ma non salva il 
*      risultato in una variabile, dammi un Warning (avviso)".
*    - Previene bug stupidi come scrivere "vettore.length();" senza fare nulla col
*      valore calcolato (sprecando preziosi cicli di CPU).
* ==========================================
*/