#include "common.h"
#include "Pools.h"
#include "Lists.h"

void*
CPtrNode::operator new(size_t){
	CPtrNode *node = CPools::GetPtrNodePool()->New();
	assert(node);
	return node;
}
void
CPtrNode::operator delete(void *p, size_t){
	CPools::GetPtrNodePool()->Delete((CPtrNode*)p);
}

// T19: ver la nota de CPtrListWalk en Lists.h.
static int odListWalkDepth;
static CPtrNode *odListPending;

bool
CPtrListDeferDelete(CPtrNode *node)
{
	if(odListWalkDepth <= 0)
		return false;
	// Se engancha por `prev` a proposito: `next` tiene que seguir siendo el
	// siguiente de la lista, porque el recorrido pudo leerlo antes de que este
	// nodo se borrase.
	node->prev = odListPending;
	odListPending = node;
	return true;
}

CPtrListWalk::CPtrListWalk(void)
{
	odListWalkDepth++;
}

CPtrListWalk::~CPtrListWalk(void)
{
	if(--odListWalkDepth > 0)
		return;
	while(odListPending){
		CPtrNode *node = odListPending;
		odListPending = node->prev;
		delete node;
	}
}

void*
CEntryInfoNode::operator new(size_t){
	CEntryInfoNode *node = CPools::GetEntryInfoNodePool()->New();
	assert(node);
	return node;
}
void
CEntryInfoNode::operator delete(void *p, size_t){
	CPools::GetEntryInfoNodePool()->Delete((CEntryInfoNode*)p);
}
