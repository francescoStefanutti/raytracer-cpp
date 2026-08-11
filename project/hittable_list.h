#pragma once

#include <memory>
#include <vector>
#include "hittable.h"

class Hittable_list : public Hittable
{
	std::vector<std::shared_ptr<Hittable>> objects;

public:
	Hittable_list() = default;

	void clear() 
	{
		objects.clear();
	}

	// guarda appunti sotto
	void add(std::shared_ptr<Hittable> object)
	{
		objects.emplace_back(std::move(object));
	}

	void reserve(size_t n)
	{
		objects.reserve(n);
	}

	bool hit(const Ray& ray, double tmin, double tmax, HitRecord& rec) const override
	{
		HitRecord temp_rec;
		bool hit_anything = false;
		double closest_so_far = tmax;

		for (const auto object : objects)
		{
			if (object->hit(ray, tmin, closest_so_far, temp_rec) == false)
				continue;
			
			hit_anything = true;
			closest_so_far = temp_rec.t;
			rec = temp_rec;
		}

		return hit_anything;
	}

};

/*
* ==========================================
* APPUNTI DA SENIOR DEV: IL PATTERN COMPOSITE E LA RICERCA DEL PIÙ VICINO
* ==========================================
* Questa classe è il motore che gestisce l'intera scena 3D. Il suo funzionamento
* si basa su due pilastri architetturali e matematici:
*
* 1. IL PATTERN COMPOSITE (L'illusione dell'Oggetto Unico)
*    Facendo ereditare `Hittable_list` dalla classe base `Hittable`, stiamo 
*    insegnando al C++ che "una lista di ostacoli è a sua volta un ostacolo".
*    Quando dal main.cpp lanceremo un raggio, la telecamera non dovrà 
*    preoccuparsi di quante sfere ci sono nel mondo. Chiamerà semplicemente
*    il metodo `.hit()` sul mondo intero, e il mondo si arrangerà a capire 
*    cosa è stato colpito. Astrazione pura.
*
* 2. L'ALGORITMO "CLOSEST SO FAR" (Occlusione visiva)
*    Un raggio sparato nello spazio potrebbe attraversare prima una sfera
*    vicina, poi una lontana. Quale colore dobbiamo disegnare sullo schermo? 
*    Ovviamente quello della sfera più vicina (che "occlude" o nasconde 
*    quella lontana). 
*    
*    Il ciclo for fa esattamente questo:
*    - Inizia con un limite di distanza massimo (tmax).
*    - Chiede al primo oggetto: "Sei stato colpito prima di tmax?". 
*      Se sì, salva le informazioni in `temp_rec` e AGGIORNA il limite 
*      `closest_so_far` con la nuova distanza appena trovata (che è più piccola).
*    - Passa al secondo oggetto e chiede: "Sei stato colpito prima di 
*      questo NUOVO limite?". 
*    - In questo modo, gli oggetti più lontani del primo che abbiamo colpito
*      vengono matematicamente scartati in automatico dentro il loro stesso 
*      metodo hit(), risparmiando tantissimi calcoli alla CPU!
*    - Alla fine del ciclo, la nostra scatola finale (`rec`) conterrà 
*      esclusivamente i dati dell'oggetto più vicino alla telecamera.
* ==========================================
*/

/*
* ==========================================
* APPUNTI DA SENIOR DEV: IL PATTERN "SINK" E GLI SMART POINTERS
* ==========================================
* Perché passiamo lo shared_ptr per valore invece che per const reference (const &)?
* Questo è un trucco di ottimizzazione del C++ moderno noto come "Sink Pattern".
* 
* Se usassimo `const std::shared_ptr<Hittable>&`:
* Il vector dovrebbe obbligatoriamente fare una COPIA del puntatore per 
* salvarselo in memoria. Copiare uno shared_ptr è un'operazione costosa perché 
* richiede di aggiornare il suo contatore interno (reference count) usando 
* istruzioni CPU speciali per il multithreading (operazioni atomiche).
* 
* Passando per valore e usando `std::move`:
* 1. Se passiamo un oggetto temporaneo (es. appena creato con std::make_shared),
*    il C++ "ruba" l'oggetto (Move Semantics) per riempire il parametro della 
*    funzione, senza incrementare il reference count (zero copie).
* 2. std::move trasferisce poi istantaneamente la proprietà di questo puntatore 
*    direttamente dentro la memoria del vector (tramite emplace_back).
* 
* Risultato: Zero copie e massima efficienza. È il modo standard in C++ per 
* dire a una funzione: "Prendi possesso definitivo di questa risorsa".
* ==========================================
*/