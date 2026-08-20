#pragma once
#include <array>
#include "vec3.h"
class Matrix4x4
{
	std::array<std::array<double, 4>, 4> m; // matrice 4x4, righe x colonne, in coordinate omogenee

public:
	Matrix4x4()
		: m{{ {1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1} }} // di default: identità (nessuna trasformazione)
	{ }

	Matrix4x4(const std::array<std::array<double, 4>, 4>& values)
		: m(values)
	{ }

	Point3 transform_point(const Point3& p) const
	{ 
		// punto: coordinata omogenea w=1 implicita (il +m[i][3] applica anche la traslazione)
		auto x = m[0][0]*p.x + m[0][1]*p.y + m[0][2]*p.z + m[0][3];
		auto y = m[1][0]*p.x + m[1][1]*p.y + m[1][2]*p.z + m[1][3];
		auto z = m[2][0]*p.x + m[2][1]*p.y + m[2][2]*p.z + m[2][3];
		return Point3{x, y, z};
	}

	Vec3 transform_vector(const Vec3& v) const
	{ 
		// vettore: coordinata omogenea w=0 implicita (niente colonna di traslazione: solo orientamento)
		auto x = m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z;
		auto y = m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z;
		auto z = m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z;
		return Vec3{x, y, z};
	}

	static Matrix4x4 translation(const Vec3& offset)
	{
		std::array<std::array<double, 4>, 4> values = { {
			{1,0,0,offset.x},
			{0,1,0,offset.y},
			{0,0,1,offset.z},
			{0,0,0,1}
			} 
		};
		return Matrix4x4(values);
	}

	static Matrix4x4 rotation_z(double teta) // rotazione attorno all'asse Z, angolo in gradi
	{
		std::array<std::array<double, 4>, 4> values = { {
			{cos(deg_to_rad(teta)),-sin(deg_to_rad(teta)),0,0},
			{sin(deg_to_rad(teta)),cos(deg_to_rad(teta)),0,0},
			{0,0,1,0},
			{0,0,0,1}
			} 
		};
		return Matrix4x4(values);
	}

	static Matrix4x4 rotation_y(double teta) // rotazione attorno all'asse Y, angolo in gradi
	{
		std::array<std::array<double, 4>, 4> values = { {
			{cos(deg_to_rad(teta)),0,sin(deg_to_rad(teta)),0},
			{0,1,0,0},
			{-sin(deg_to_rad(teta)),0,cos(deg_to_rad(teta)),0},
			{0,0,0,1}
			} 
		};
		return Matrix4x4(values);
	}

	static Matrix4x4 rotation_x(double teta) // rotazione attorno all'asse X, angolo in gradi
	{
		std::array<std::array<double, 4>, 4> values = { {
			{1,0,0,0},
			{0,cos(deg_to_rad(teta)),-sin(deg_to_rad(teta)),0},
			{0,sin(deg_to_rad(teta)),cos(deg_to_rad(teta)),0},
			{0,0,0,1}
			} 
		};
		return Matrix4x4(values);
	}

	static Matrix4x4 rotation(double ax, double ay, double az) // rotazione combinata, ordine Rx*Ry*Rz
	{
		return rotation_x(ax) * rotation_y(ay) * rotation_z(az);
	}

	static Matrix4x4 scale(double sx, double sy, double sz) // matrice diagonale: scala indipendentemente sui tre assi
	{
		std::array<std::array<double, 4>, 4> values = { {
			{sx,0,0,0},
			{0,sy,0,0},
			{0,0,sz,0},
			{0,0,0,1}
			} 
		};
		return Matrix4x4(values);
	}

	Matrix4x4 operator*(const Matrix4x4& other) const // composizione di due trasformazioni (prima other, poi this)
	{
		std::array<std::array<double,4>,4> r = {{ {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0} }};
		for(int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				for(int k = 0; k<4; k++ )
				{
					r[i][j] += m[i][k] * other.m[k][j];
				}
			}
		}
		return r;
	}
};

struct RigidMatrix // trasformazione rigida (rotazione+traslazione, niente scaling): forward e inverse sempre coerenti
{
	Matrix4x4 forward;  // spazio locale -> spazio mondo
	Matrix4x4 inverse;  // spazio mondo -> spazio locale

	static RigidMatrix translation(const Vec3& offset) 
	{
		return { Matrix4x4::translation(offset), Matrix4x4::translation(-offset) };
	}

	static RigidMatrix rotation(double ax, double ay, double az) 
	{
		return { Matrix4x4::rotation(ax, ay, az), Matrix4x4::rotation_z(-az)*Matrix4x4::rotation_y(-ay)*Matrix4x4::rotation_x(-ax) };
	}

	RigidMatrix operator*(const RigidMatrix& other) const // compone due RigidMatrix mantenendo forward/inverse coerenti
	{
		return{ forward * other.forward, other.inverse*inverse };
	}
};

// trasformazione generale (rotazione+traslazione+scale): oltre a forward/inverse, tiene anche la matrice dedicata alle normali
struct GeneralMatrix 
{
	Matrix4x4 forward;  // spazio locale -> spazio mondo (punti)
	Matrix4x4 inverse;  // spazio mondo -> spazio locale (per portare il raggio all'indietro)
	Matrix4x4 normal;   // inversa-trasposta della parte lineare di forward, dedicata solo alle normali

	static GeneralMatrix translation(const Vec3& offset) 
	{
		return { Matrix4x4::translation(offset), Matrix4x4::translation(-offset), Matrix4x4()}; // niente scale/rotazione: le normali non cambiano, normal = identità
	}

	static GeneralMatrix rotation(double ax, double ay, double az) 
	{
		return { Matrix4x4::rotation(ax, ay, az), Matrix4x4::rotation_z(-az)*Matrix4x4::rotation_y(-ay)*Matrix4x4::rotation_x(-ax),  Matrix4x4::rotation(ax, ay, az) }; // rotazione pura: normal coincide con forward
	}

	static GeneralMatrix scale(double sx, double sy, double sz) 
	{
		assert(sx != 0 && sy != 0 && sz != 0 && "GeneralMatrix::scale: componente zero non invertibile");
		return { Matrix4x4::scale(sx, sy, sz), Matrix4x4::scale(1/sx, 1/sy, 1/sz), Matrix4x4::scale(1/sx, 1/sy, 1/sz) }; // scale puro: normal coincide con inverse (diagonale = simmetrica)
	}

	GeneralMatrix operator*(const GeneralMatrix& other) const // compone due GeneralMatrix mantenendo forward/inverse/normal coerenti
	{
		return{ forward * other.forward, other.inverse*inverse, normal*other.normal};
	}
};

/*
* ==========================================
* COME FUNZIONA matrix.h
* ==========================================
* Matrix4x4 rappresenta una trasformazione geometrica generica in coordinate
* omogenee 4x4: un punto/vettore 3D viene esteso con una quarta coordinata (w),
* che vale 1 per i punti (subiscono la traslazione, vedi transform_point) e 0
* per i vettori/direzioni (non subiscono traslazione, solo rotazione/orientamento,
* vedi transform_vector). Questo è il motivo per cui esistono due metodi diversi
* invece di uno solo: un punto e un vettore si comportano diversamente sotto
* trasformazione, anche se in memoria hanno la stessa forma (x,y,z).
*
* Le tre funzioni rotation_x/y/z applicano la formula standard di rotazione
* rigida (destrorsa) attorno al rispettivo asse: ciascuna lascia invariata la
* coordinata dell'asse su cui si ruota, e mescola le altre due secondo seno e
* coseno dell'angolo. Sono coerenti tra loro (stessa convenzione di segno),
* condizione necessaria perché comporle in sequenza (rotation_x * rotation_y *
* rotation_z, vedi rotation()) dia un risultato prevedibile: un angolo positivo
* ruota sempre nello stesso verso "logico", qualunque sia l'asse scelto.
*
* scale() è una matrice diagonale (sx, sy, sz sulla diagonale principale):
* ridimensiona indipendentemente lungo i tre assi locali, senza alterare
* rotazione o traslazione. Componendo scale con rotazione/traslazione (vedi
* GeneralMatrix) si ottiene una trasformazione affine generale.
*
* operator* compone due matrici con la normale moltiplicazione riga-per-colonna:
* (A*B) applicata a un punto equivale prima ad applicare B, poi A (l'ordine di
* scrittura è "da destra a sinistra" nell'effetto reale) — per questo, per
* ruotare un oggetto attorno al proprio centro invece che attorno all'origine,
* si compone come T(centro) * R * T(-centro): prima si porta l'oggetto
* nell'origine (T(-centro), applicata per ultima), poi si ruota (R), poi lo si
* riporta al suo posto (T(centro), applicata per prima).
*
* RigidMatrix tiene sempre accoppiate una matrice diretta (forward) e la sua
* inversa (inverse), calcolate insieme dalle stesse factory statiche
* (translation/rotation), così chi usa la libreria non deve mai invertire una
* matrice a mano: per una traslazione l'inversa è la traslazione opposta, per
* una rotazione è la stessa rotazione con angoli negati e ordine invertito
* (Rz(-c)*Ry(-b)*Rx(-a) è l'inversa di Rx(a)*Ry(b)*Rz(c), proprietà delle
* rotazioni rigide: l'inversa di una composizione è la composizione delle
* inverse in ordine contrario). operator* su RigidMatrix compone due
* trasformazioni mantenendo questa coerenza automaticamente su entrambe le
* matrici, forward e inverse, in un solo passaggio.
*
* GeneralMatrix estende lo stesso principio aggiungendo un terzo membro, normal:
* la matrice da usare (al posto di forward) quando si trasforma una NORMALE
* invece di un punto, necessaria non appena la trasformazione include uno scale
* (per una trasformazione puramente rigida, forward e normal coincidono sempre,
* vedi rotation() qui sotto). La regola geometrica è "inversa-trasposta della
* parte lineare 3x3 di forward", ma GeneralMatrix non calcola mai un'inversione
* di matrice generica: sfrutta il fatto che ogni pezzo elementare (traslazione,
* rotazione, scale) ha un'inversa-trasposta nota in anticipo e facile da scrivere
* a mano:
* - traslazione: non ha parte lineare che tocchi le normali -> normal = identità.
* - rotazione: l'inversa di una rotazione è la sua trasposta, quindi
*   inversa-trasposta = la rotazione stessa -> normal coincide con forward.
* - scale: una matrice diagonale è simmetrica (trasposta di se stessa), quindi
*   inversa-trasposta si riduce allo scale reciproco -> normal coincide con
*   inverse (1/sx, 1/sy, 1/sz), senza bisogno di trasporre nulla esplicitamente.
* operator* di GeneralMatrix compone normal con lo STESSO ordine di forward
* (this*other, non invertito come inverse): normal e forward giocano lo stesso
* ruolo "verso il mondo", solo su entità diverse (vettori/normali vs punti),
* mentre inverse rappresenta il percorso opposto "verso lo spazio locale" e per
* questo si compone in ordine contrario.
* ==========================================
*/