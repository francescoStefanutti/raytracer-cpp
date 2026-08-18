#pragma once
#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "aabb.h"
#include "ray.h"
#include <memory>

// Parallelogramma definito da un angolo Q e due vettori-lato u, v: P(a,b) = Q + a*u + b*v
class Quad : public Hittable
{
	Point3 Q;
	Vec3 u, v;
	std::shared_ptr<Material> material;
	Vec3 n;       // normale del piano (non normalizzata), n = u x v
	Vec3 w_aux;   // vettore ausiliario per ricavare alfa/beta da un punto sul piano
	double D;     // termine noto dell'equazione del piano: dot(n, P) = D
	AABB bbox;    // bounding box precalcolato una sola volta, invece che ad ogni chiamata

public:
	Quad(const Point3& Q, Vec3 u, Vec3 v, std::shared_ptr<Material> material)
		: Q(Q), u(u), v(v), material(material), n(cross(u, v)), w_aux(n / dot(n, n)), D(dot(n, Q)), bbox({ AABB{Q, Q + u + v}, AABB{Q + u, Q + v} }) // unione dei box sulle due diagonali, calcolata una sola volta qui
	{ }

	bool hit(const Ray& ray, double tmin, double tmax, HitRecord& rec) const override
	{
		auto denom = dot(n, ray.direction());

		if (std::fabs(denom) < 1e-8) // raggio ~parallelo al piano: nessuna intersezione utile
			return false;
		else
		{
			auto t = (D - dot(n, ray.origin())) / denom; // intersezione raggio-piano infinito
			if (t < tmin || t > tmax)
				return false;
			else
			{
				auto P = ray.at(t); // punto di intersezione sul piano
				auto alfa = dot(w_aux, cross(P - Q, v)); // coordinata lungo u
				auto beta = dot(w_aux, cross(u, P - Q)); // coordinata lungo v

				// il controllo bordi/UV è delegato a is_interior(), virtual: le sottoclassi (es. un triangolo) possono riusare tutto hit() ridefinendo solo quel controllo
				if (!is_interior(alfa, beta, rec))
					return false;
				else
				{
					rec.t = t;
					rec.P = P;
					rec.mat = material;
					rec.set_face_normal(ray, unit_vector(n)); // normalizzata qui, non tenuta come membro
					return true;
				}
			}
		}
	}

	AABB bounding_box() const override
	{
		return bbox; // ora solo un accesso al membro, nessun ricalcolo
	}

	// Controllo dei bordi per un parallelogramma: dentro se 0<=alfa<=1 e 0<=beta<=1.  Virtual: una sottoclasse può ridefinirlo per ottenere un'altra forma 2D (es. triangolo, disco) riusando comunque tutta la logica di intersezione col piano scritta in hit().
	virtual bool is_interior(double alfa, double beta, HitRecord& rec) const
	{
		if (alfa < 0 || alfa > 1 || beta < 0 || beta > 1) // fuori dai bordi del parallelogramma
			return false;
		else 
		{
			rec.u = alfa; // per un quadrilatero, le UV coincidono con alfa/beta
			rec.v = beta;
			return true;
		}
	}
};

std::shared_ptr<Hittable> box(const Point3& a, const Point3& b, const std::shared_ptr<Material>& material)
{
	auto sides = std::make_shared<Hittable_list>();

	auto dx = b.x - a.x;
	auto dy = b.y - a.y;
	auto dz = b.z - a.z;

	// faccia a z minima (a.z): normale verso -z
	sides->add(std::make_shared<Quad>(a, Vec3{ 0,dy,0 }, Vec3{ dx,0,0 }, material));
	// faccia a y minima (a.y): normale verso -y
	sides->add(std::make_shared<Quad>(a, Vec3{ dx,0,0 }, Vec3{ 0,0,dz }, material));
	// faccia a x minima (a.x): normale verso -x
	sides->add(std::make_shared<Quad>(a, Vec3{ 0,0,dz }, Vec3{ 0,dy,0 }, material));
	// faccia a z massima (b.z): normale verso +z
	sides->add(std::make_shared<Quad>(b, Vec3{ -dx,0,0 }, Vec3{ 0,-dy,0 }, material));
	// faccia a y massima (b.y): normale verso +y
	sides->add(std::make_shared<Quad>(b, Vec3{ 0,0,-dz }, Vec3{ -dx,0,0 }, material));
	// faccia a x massima (b.x): normale verso +x
	sides->add(std::make_shared<Quad>(b, Vec3{ 0,-dy,0 }, Vec3{ 0,0,-dz }, material));

	return sides;
}


/*
* RIEPILOGO DEL FILE
*
* Quad rappresenta un parallelogramma nello spazio, definito da un angolo Q
* e due vettori-lato u, v che escono da Q. Ogni punto della sua superficie
* è P(a,b) = Q + a*u + b*v, con a,b in [0,1] per restare dentro i bordi.
*
* hit(): prima trova l'intersezione con il PIANO infinito su cui giace il
* parallelogramma (equazione dot(n,P) = D, con n = u x v normale del piano
* e D = dot(n,Q) termine noto). Risolvendo per t si ottiene una formula
* diretta (nessuna quadratica come per la sfera): un denominatore vicino a
* zero significa raggio parallelo al piano, quindi nessun hit.
*
* Trovato il punto P sul piano, si calcolano le coordinate alfa/beta che
* esprimono P nella base (u,v) a partire da Q, sfruttando il vettore
* ausiliario w_aux = n / dot(n,n) e alcune identità del prodotto vettoriale
* (risultato standard di geometria computazionale, preso "as-is" dal libro).
* Se alfa o beta escono da [0,1], il punto è sul piano ma fuori dai bordi
* reali del parallelogramma, quindi niente hit.
*
* bounding_box(): il parallelogramma ha spessore zero lungo l'asse
* eventualmente perpendicolare al suo piano, il che romperebbe il test
* "slab" di AABB::hit() (intervallo di dimensione zero). Per questo il
* costruttore AABB(Point3,Point3) applica Interval::expand() su ogni asse,
* garantendo uno spessore minimo. Il box finale è l'unione dei box costruiti
* sulle due diagonali del parallelogramma (Q<->Q+u+v e Q+u<->Q+v), necessaria
* per coprire correttamente qualsiasi orientamento nello spazio.
*
* MODIFICHE RISPETTO ALLA PRIMA VERSIONE (ottimizzazioni + estendibilità):
*
* 1. n resta la normale GREZZA (non normalizzata) come membro. È stato
*    provato a salvarla già normalizzata per risparmiare la normalizzazione
*    ripetuta in hit(), ma w_aux = n/dot(n,n) dipende dalla lunghezza del
*    cross product grezzo: se n fosse già unitaria, dot(n,n)=1 sempre, e
*    w_aux perderebbe il fattore di scala corretto, rompendo alfa/beta.
*    Si è scelto quindi di tenere n grezza e normalizzare solo al momento
*    dell'uso in set_face_normal(), accettando quel piccolo costo ripetuto
*    pur di non introdurre un bug o un membro ridondante.
*
* 2. bbox è ora un membro, calcolato una sola volta nel costruttore invece
*    che ricalcolato ad ogni chiamata di bounding_box() (utile perché la
*    BVH può interrogare il bounding box più volte durante la costruzione).
*
* 3. Il controllo "il punto è dentro i bordi?" è stato estratto dal corpo
*    di hit() in un metodo virtual separato, is_interior(alfa, beta, rec).
*    hit() si occupa solo di trovare il piano e calcolare alfa/beta (comune
*    a qualsiasi primitivo piatto); is_interior() decide la FORMA vera e
*    propria e imposta le UV di conseguenza. Questo permette a una futura
*    sottoclasse (es. Triangle) di ereditare hit() così com'è e ridefinire
*    solo is_interior() con la propria condizione (es. alfa>0 && beta>0 &&
*    alfa+beta<1 per un triangolo), senza duplicare la logica di intersezione
*    col piano.
*/