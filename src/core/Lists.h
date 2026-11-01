#pragma once

class CPtrNode
{
public:
	void *item;
	CPtrNode *prev;
	CPtrNode *next;

	void *operator new(size_t);
	void operator delete(void *p, size_t);
};

// T19: un nodo NO se devuelve al pool mientras se recorre una lista.
// CWorld::Remove() -> RemoveFromMovingList() borra el nodo en mitad del
// recorrido de CWorld::Process (p.ej. un peaton que termina de morir), y el
// pool recicla ese mismo hueco para el siguiente InsertItem; desde ahi el
// recorrido salta a la cabeza de la lista y vuelve a procesar lo ya procesado
// sin terminar nunca. Es el cuelgue con mark=137: hilo a tope, sin JSERR, sin
// OOM y con el perro guardian vivo (14 s de silencio en la traza). Con esto el
// nodo se desengancha en el acto y su memoria se suelta al salir del ultimo
// recorrido, asi que ningun `next` ya leido puede cambiar de dueno.
bool CPtrListDeferDelete(CPtrNode *node);

class CPtrListWalk
{
public:
	CPtrListWalk(void);
	~CPtrListWalk(void);
};

class CPtrList
{
public:
	CPtrNode *first;

	CPtrList(void) { first = nil; }
	~CPtrList(void) { Flush(); }
	CPtrNode *FindItem(void *item){
		CPtrNode *node;
		for(node = first; node; node = node->next)
			if(node->item == item)
				return node;
		return nil;
	}
	CPtrNode *InsertNode(CPtrNode *node){
		node->prev = nil;
		node->next = first;
		if(first)
			first->prev = node;
		first = node;
		return node;
	}
	CPtrNode *InsertItem(void *item){
		CPtrNode *node = new CPtrNode;
		node->item = item;
		InsertNode(node);
		return node;
	}
	void RemoveNode(CPtrNode *node){
		if(node == first)
			first = node->next;
		if(node->prev)
			node->prev->next = node->next;
		if(node->next)
			node->next->prev = node->prev;
	}
	void DeleteNode(CPtrNode *node){
		RemoveNode(node);
		if(CPtrListDeferDelete(node))
			return;
		delete node;
	}
	void RemoveItem(void *item){
		CPtrNode *node, *next;
		for(node = first; node; node = next){
			next = node->next;
			if(node->item == item)
				DeleteNode(node);
		}
	}
	void Flush(void){
		CPtrNode *node, *next;
		for(node = first; node; node = next){
			next = node->next;
			DeleteNode(node);
		}
	}
};

class CSector;

// This records in which sector list a Physical is
class CEntryInfoNode
{
public:
	CPtrList *list;		// list in sector
	CPtrNode *listnode;	// node in list
	CSector *sector;

	CEntryInfoNode *prev;
	CEntryInfoNode *next;

	void *operator new(size_t);
	void operator delete(void *p, size_t);
};

class CEntryInfoList
{
public:
	CEntryInfoNode *first;

	CEntryInfoList(void) { first = nil; }
	~CEntryInfoList(void) { Flush(); }
	CEntryInfoNode *InsertNode(CEntryInfoNode *node){
		node->prev = nil;
		node->next = first;
		if(first)
			first->prev = node;
		first = node;
		return node;
	}
	CEntryInfoNode *InsertItem(CPtrList *list, CPtrNode *listnode, CSector *sect){
		CEntryInfoNode *node = new CEntryInfoNode;
		node->list = list;
		node->listnode = listnode;
		node->sector = sect;
		InsertNode(node);
		return node;
	}
	void RemoveNode(CEntryInfoNode *node){
		if(node == first)
			first = node->next;
		if(node->prev)
			node->prev->next = node->next;
		if(node->next)
			node->next->prev = node->prev;
	}
	void DeleteNode(CEntryInfoNode *node){
		RemoveNode(node);
		delete node;
	}
	void Flush(void){
		CEntryInfoNode *node, *next;
		for(node = first; node; node = next){
			next = node->next;
			DeleteNode(node);
		}
	}
};
