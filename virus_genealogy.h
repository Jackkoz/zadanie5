#ifndef _VIRUS_GENEALOGY_H_
#define _VIRUS_GENEALOGY_H_

#include <boost/shared_ptr.hpp>
#include <exception>
#include <vector>
#include <map>
#include <set>

class VirusNotFound: public std::exception
{
    virtual const char* what() const throw()
    {
        return "VirusNotFound";
    }
} VirusNotFound;

class VirusAlreadyCreated: public std::exception
{
    virtual const char* what() const throw()
    {
        return "VirusAlreadyCreated";
    }
} VirusAlreadyCreated;

class TriedToRemoveStemVirus: public std::exception
{
    virtual const char* what() const throw()
    {
        return "TriedToRemoveStemVirus";
    }
} TriedToRemoveStemVirus;

template <class Virus>
class VirusGenealogy;

template <class Virus>
class GenealogyNode
{
private:
    typedef typename Virus::id_type id_type; // Typ identyfikatora wirusów
    typedef GenealogyNode<Virus> Node; // Typ węzła w grafie pokrewieństwa

    Virus* virus; // przechowywany egzemplarz wirusa
    id_type virus_id; // identyfikator wirusa

    std::set<std::shared_ptr<Node>> children; // zbiór potomków wirusa
    std::set<Node*> parents; // zbiór przodków wirusa

    // iterator węzła w mapie 'virus_collection'
    // typename std::map<id_type, Node>::iterator my_iterator;
    // Powyższe nigdzie nie było używane

    std::weak_ptr<Node> myself;

    friend class VirusGenealogy<Virus>;

public:
    GenealogyNode(id_type id) : virus_id(id)
    {
        virus = new Virus(id);
        // być może zostanie rzucony wyjątek podczas przypisania lub konstrukcji
    }

    GenealogyNode(const GenealogyNode& other)
    : virus_id(other.virus_id), children(other.children), parents(other.parents)
    {
        virus = new Virus(*other.virus);
        // być może zostanie rzucony wyjątek podczas przypisania lub konstrukcji
    }

    ~GenealogyNode()
    {
        delete virus;
    }
};

template <class Virus>
class VirusGenealogy
{
private:
    // Typ identyfikatora wirusów
    typedef typename Virus::id_type id_type;

    // Typ węzła w grafie pokrewieństwa
    typedef GenealogyNode<Virus> Node;

    // Typ iteratora mapy 'virus_collection'
    typedef typename std::map<id_type, Node>::iterator it;

    // Węzeł wirusa macierzystego
    Node* stem;

    // Lista kontrolowanych wirusów
    std::map<id_type, Node> virus_collection;

public:
    // Tworzy nową genealogię.
    // Tworzy także węzęł wirusa macierzystego o identyfikatorze stem_id.
    VirusGenealogy(id_type const &stem_id)
    {
        Node node(stem_id);
        // jeśli konstruktor rzucił wyjątek, zostanie on wyrzucony na zewnątrz

        std::pair<id_type, Node> elt(stem_id, node);
        // jeśli podczas konstrukcji został rzucony wyjątek, zostanie on wyrzucony na zewnątrz

        std::pair<it, bool> my_iterator = virus_collection.insert(elt);
        // ze względu na silną odporność na wyjątki operacji insert,
        // element został wstawiony lub został rzucony wyjątek

        // odpowienia modyfikacja pola my_iterator wstawionego elementu
        // my_iterator.first->second.my_iterator = my_iterator.first;

        // modyfikacja pola 'stem'
        stem = &(my_iterator.first->second);
    }

    // Zwraca identyfikator wirusa macierzystego.
    id_type get_stem_id() const
    {
        return stem->virus_id;
    }

    // Zwraca listę identyfikatorów bezpośrednich następników wirusa
    // o podanym identyfikatorze.
    // Zgłasza wyjątek VirusNotFound, jeśli dany wirus nie istnieje.
    std::vector<id_type> get_children(id_type const &id) const
    {
        typename std::map<id_type, Node>::const_iterator pos = virus_collection.find(id);
        // jeśli został rzucony wyjątek, wyleci na zewnątrz

        if (pos == virus_collection.end()) // id nie istnieje w mapie
            throw VirusNotFound;

        std::vector<id_type> result;
        typename std::set<Node*>::iterator i;

        for (i = pos->second.children.begin(); i != pos->second.children.end(); i++)
            result.push_back((*i)->virus_id);
        // jeśli został rzucony wyjątek, wyleci na zewnątrz

        return result;
    }

    // Zwraca listę identyfikatorów bezpośrednich poprzedników wirusa
    // o podanym identyfikatorze.
    // Zgłasza wyjątek VirusNotFound, jeśli dany wirus nie istnieje.
    std::vector<id_type> get_parents(id_type const &id) const
    {
        typename std::map<id_type, Node>::const_iterator pos = virus_collection.find(id);
        // jeśli został rzucony wyjątek, wyleci na zewnątrz

        if (pos == virus_collection.end()) // id nie istnieje w mapie
            throw VirusNotFound;

        std::vector<id_type> result;
        typename std::set<Node*>::iterator i;

        for (i = pos->second.parents.begin(); i != pos->second.parents.end(); i++)
            result.push_back((*i)->virus_id);
        // jeśli został rzucony wyjątek, wyleci na zewnątrz

        return result;
    }

    // Sprawdza, czy wirus o podanym identyfikatorze istnieje.
    // W ten sposób można zrobić wersję no-throw exists, ale czy tak chcemy?
    // Poprzednie rzeczy są napisane tak jakby throw mogło rzucać
    bool exists(id_type const &id) const
    {
        // w przypadku rzucenia wyjątku przez operacje porównania,
        // funkcja wyrzuci go na zewnątrz
        return virus_collection.count(id) == 1;
    }

    // Zwraca referencję do obiektu reprezentującego wirus o podanym
    // identyfikatorze.
    // Zgłasza wyjątek VirusNotFound, jeśli żądany wirus nie istnieje.
    Virus& operator[](id_type const &id) const
    {
        typename std::map<id_type, Node>::const_iterator pos = virus_collection.find(id);
        // jeśli został rzucony wyjątek, wyleci na zewnątrz

        if (pos == virus_collection.end()) // id nie istnieje w mapie
            throw VirusNotFound;

        return *pos->second.virus;
    }

    // Tworzy węzęł reprezentujący nowy wirus o identyfikatorze id
    // powstały z wirusów o podanym identyfikatorze parent_id lub
    // podanych identyfikatorach parent_ids.
    // Zgłasza wyjątek VirusAlreadyCreated, jeśli wirus o identyfikatorze
    // id już istnieje.
    // Zgłasza wyjątek VirusNotFound, jeśli któryś z wyspecyfikowanych
    // poprzedników nie istnieje.
    void create(id_type const &id, id_type const &parent_id)
    {
        create(id, std::vector<id_type>(1, parent_id));
        // silna odporność konstruktora wektora na wyjątki
    }

    void create(id_type const &id, std::vector<id_type> const &parent_ids)
    {
        // wektor przodków jest pusty
        if (parent_ids.empty())
            throw VirusNotFound;

        Node node(id);
        // jeśli operator przypisania rzucił wyjątek, będzie on wyrzucony na zewnątrz

        typename std::vector<id_type>::const_iterator i;
        std::vector<Node*> parents;

        // Poniższe usunie się na koniec i use_count będzie właściwe
        std::shared_ptr<Node> new_ptr = std::shared_ptr<Node>(node);
        // Na poczet przyszłych nowych shared_ptr w connect i operacji w remove
        node.myself = std::weak_ptr<Node>(new_ptr);

        for (i = parent_ids.begin(); i != parent_ids.end(); i++)
        {
            it pos = virus_collection.find(*i);
            // jeśli został rzucony wyjątek, wyleci na zewnątrz

            if (pos == virus_collection.end()) // wirus o podanym id nie istnieje
                throw VirusNotFound;

            node.parents.insert(&pos->second.myself);
            // Czy to nie jest niepotrzebne kopiowanie danych?
            // Dokładnie to samo co w parents jest w node.parents
            parents.push_back(&pos->second);
        }
        // jeśli podczas dodawania elementów do seta zostanie rzucony wyjątek,
        // zostanie wyrzucony na zewnątrz

        std::pair<id_type, Node> elt(id, node);
        // jeśli podczas konstrukcji został rzucony wyjątek,
        // będzie on rzucony również przez konstruktor

        std::pair<it, bool> my_iterator = virus_collection.insert(elt);
        // ze względu na silną odporność na wyjątki operacji insert,
        // element został wstawiony lub został rzucony wyjątek

        // odpowienia modyfikacja pola my_iterator wstawionego elementu
        // my_iterator.first->second.my_iterator = my_iterator.first;

        // poinformowanie rodziców o nowym potomku
        typename std::vector<Node*>::const_iterator j;

        for (j = parents.begin(); j != parents.end(); j++)
            (*j)->children.insert(std::shared_ptr<Node>(new_ptr));
    }

    // Dodaje nową krawędź w grafie genealogii.
    // Zgłasza wyjątek VirusNotFound, jeśli któryś z podanych wirusów nie istnieje.
    void connect(id_type const &child_id, id_type const &parent_id)
    {
        it child = virus_collection.find(child_id);
        // jeśli został rzucony wyjątek, wyleci na zewnątrz

        if (child == virus_collection.end())
            throw VirusNotFound;

        it parent = virus_collection.find(parent_id);
        // jeśli został rzucony wyjątek, wyleci na zewnątrz

        if (parent == virus_collection.end())
            throw VirusNotFound;



        // operacje nie rzucające wyjątków
        child->second.parents.insert(&parent->second.myself);
        parent->second.children.insert(std::shared_ptr<Node>(&child->second.myself));
    }

    // Usuwa wirus o podanym identyfikatorze.
    // Zgłasza wyjątek VirusNotFound, jeśli żądany wirus nie istnieje.
    // Zgłasza wyjątek TriedToRemoveStemVirus przy próbie usunięcia
    // wirusa macierzystego.
    void remove(id_type const &id)
    {
        if (id == stem->virus_id)
            throw TriedToRemoveStemVirus;
        // jeśli operator porównania rzuci wyjątek, wyleci on z metody

        it pos = virus_collection.find(id);
        // jeśli podczas wyszukiwania zostanie rzucony wyjątek, wyleci on z metody

        if (pos == virus_collection.end()) // wirus o podanym id nie istnieje
            throw VirusNotFound;

        std::shared_ptr<Node> self(pos->second.myself);

        // Tu trzeba usunąć wszystkie shared_ptr z setów dzieci (dla których ten jest dzieckiem)
        // I z parents tych, dla których nie jest jedynym rodzicem
        for (auto parent : pos->second.parents)
            parent.children.erase(self);

        for (auto child : pos->second.children)
            child.parents.erase(pos->second.myself);

        virus_collection.erase(pos); // realne usunięcie wirusa z mapy, wywołany destruktor
    }
};

#endif
