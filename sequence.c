#include <stdio.h>
#include <stdlib.h>
#include "skel.h"


AVLTree* avlCreateTree(uint32_t (*comp) (uint32_t,uint32_t))
{
	AVLTree* newTree = (AVLTree*) malloc(sizeof(AVLTree));
	newTree->comp = comp;
	newTree->root = NULL;
	newTree->size = 0;

	return newTree;
}

AVLNode* avlNewNode(AVLTree* tree){
	AVLNode* newNode = (AVLNode*) malloc(sizeof(AVLNode));
	
	newNode->p = newNode->r = newNode->l = NULL;
	newNode->height = 1;

	return newNode;
}

int max(int a, int b){
	return (a > b)? a : b;
}

void avlRightRotate(AVLTree *tree,  AVLNode *y){
	
	AVLNode* x = y->l;
	int a, b, c, d;

	if(y->p == NULL){
		x->p = y->p;
		y->l = x->r;
		y->p = x;
		x->r = y;
		tree->root = x;
		a = b = c = d = 0;

		if(y->l != NULL)
		a = y->l->height;
		if(y->r != NULL)
			b = y->r->height;
		if(x->l != NULL)
			c = x->l->height;
		if(x->r != NULL)
			c = x->r->height;

		y->height = max(a, b)+1;
		x->height = max(c, d)+1;
		return;
	}
	y->l = x->r;

	if (x->r != NULL)
		(x->r)-> p = y;

	x->p = y ->p;

	if (y->p->l == y)
		y->p->l = x;
	else 
		y->p->r = x;

	x->r = y;
	y->p = x;
	a = b = c = d = 0;

	if(y->l != NULL)
		a = y->l->height;
	if(y->r != NULL)
		b = y->r->height;
	if(x->l != NULL)
		c = x->l->height;
	if(x->r != NULL)
		c = x->r->height;

	y->height = max(a, b)+1;
	x->height = max(c, d)+1;

	return;
}

void avlLeftRotate(AVLTree *tree, AVLNode *x){

	int a, b, c, d;
	AVLNode* y = x->r;

	if(x->p == NULL){
		y->p = x->p;
		x->r = y->l;
		x->p = y;
		y->l = x;
		tree->root = y;

		a = b = c = d = 0;
		if(y->l != NULL)
			a = y->l->height;
		if(y->r != NULL)
			b = y->r->height;
		if(x->l != NULL)
			c = x->l->height;
		if(x->r != NULL)
			c = x->r->height;

		y->height = max(a, b)+1;
		x->height = max(c, d)+1;
		
		return;
	}	
	x->r = y->l;

	if (y->l != NULL)
		y->l->p = x;
	
	y->p = x->p;

	if (x->p->l == x)
		x->p->l = y;
	else
		x->p->r = y;

	y->l = x;
	x->p = y;
	a = b = c = d = 0;

	if(y->l != NULL)
		a = y->l->height;
	if(y->r != NULL)
		b = y->r->height;
	if(x->l != NULL)
		c = x->l->height;
	if(x->r != NULL)
		c = x->r->height;

	y->height = max(a, b)+1;
	x->height = max(c, d)+1;

	return;
}

int avlGetBalance(AVLNode *x){
	if (x == NULL)
		return 0;
	int a, b;
	a = b = 0;

	if(x->l != NULL)
		a = x->l->height;
	if(x->r != NULL)
		b = x->r->height;
	return (a - b);
}
void avlFixup(AVLTree* tree, AVLNode* node, uint32_t index){
	
	AVLNode* y = node;
	while (y != NULL){
	
		int a, b;
		a = b = 0;

		if(y->l != NULL)
			a = y->l->height;
		if(y->r != NULL)
			b = y->r->height;

		y->height = max(a,b) + 1;
		int balance = avlGetBalance(y);
		uint32_t c, d;
		c = d = 0;

		if(y->l != NULL)
			c = y->l->index;
		if(y->r != NULL)
			d = y->r->index;

		if ((balance > 1) && (tree->comp(index, c) == -1))
			avlRightRotate(tree, y);
		if ((balance < -1) && (tree->comp(index, d) == 1))
			avlLeftRotate(tree, y);
		if ((balance > 1) && (tree->comp(index, c)== 1)){
			avlLeftRotate(tree, y->l);
			avlRightRotate(tree, y);
		}
		if ((balance < -1) && (tree->comp(index, d) == -1)){
			avlRightRotate(tree, y->r);
			avlLeftRotate(tree, y);
		}
		y = y->p;
	}
}

void avlInsert(struct AVLTree* tree, rtable_entry elem, uint32_t index){

	AVLNode* z = avlNewNode(tree);
	z->entry = elem;
	z->index = index;

	AVLNode* x  = tree->root;

	if(x == NULL){
		tree->root = z;
		tree->size++;
		return;
	}

	
	AVLNode* y  = tree->root->p;

	while(x != NULL){
		y = x;
		if (tree->comp(x->index, index) == 1)
			x = x->l;
		else 
			if (tree->comp(x->index, index) == -1)
				x = x->r;
	}
	z->p = y;

	if((tree->comp(y->index, z->index) == 1))
	 	y->l = z;
	 else
		y->r = z;

	tree->size++;

	avlFixup(tree, y, index);
}
void avlDelFixup(AVLTree* tree,AVLNode* x){	
	int a, b;
	a = b = 0;
	
	if(x->l != NULL)
		a = x->l->height;
	if(x->r != NULL)
		b = x->r->height;
	x->height = max(a,b)+1;
	int balance = avlGetBalance(x);

	if ((balance > 1) && avlGetBalance(x->l) >=0)
			avlRightRotate(tree, x);
	if ((balance > 1) && avlGetBalance(x->l)< 0){
		avlLeftRotate(tree, x->l);
		avlRightRotate(tree, x);
	}
	if ((balance < -1) && (avlGetBalance(x->r) <= 0)){
		avlLeftRotate(tree, x);
	}
	if ((balance < -1) && avlGetBalance(x->r)>0){
		avlRightRotate(tree, x->r);
		avlLeftRotate(tree, x);
	}
	avlFixup(tree, x, x->index);
}
void AVLDelNode(struct AVLTree* tree, uint32_t index){
	AVLNode* x  = tree->root;
	AVLNode* y  = tree->root->p;

	while(x->index != index){
		y = x;
		if (tree->comp(x->index, index) == 1){
			x = x->l;
		}
		else if (tree->comp(x->index, index) == -1){
			x = x->r;
		}
	}

	if(x->l == NULL){
		if(y->l == x)
			y->l = x->r;
		else 
			y->r = x->r;
		
		avlDelFixup(tree, y);
		tree->size--;
	
		return;
	}

	if(x->r == NULL){
		if(y->l == x)
			y->l = x->l;
		else
			y->r = x->l;
		avlDelFixup(tree, y);
		tree->size--;

		return;
	}
	AVLNode* min = x->l;

	while(min->r != NULL)
		min = min->r;

	AVLDelNode(tree,min->index);
	x->index = min->index;
	avlDelFixup(tree, x);
}



void print_tree(AVLNode* node){
	if (node == NULL) 
		return;
	struct in_addr ip_addr, ip_addr2, ip_addr3; 
	ip_addr.s_addr = node->entry.prefix;
	print_tree(node->l);
	printf("%s ", inet_ntoa(ip_addr));
	ip_addr2.s_addr = node->entry.nexthop;
	printf("%s ", inet_ntoa(ip_addr2));
	ip_addr3.s_addr = node->entry.mask;
	printf("%s ", inet_ntoa(ip_addr3));
	printf("%d\n", node->entry.interface);
	print_tree(node->r);

	return;
}

rtable_entry* search(AVLNode* node, uint32_t index){
	if(node == NULL)
		return NULL;	
	
	rtable_entry* a;
	if(node->index == index)
		return &node->entry;
	if(index < node->index)
		a = search(node->l, index);
	else
		a = search(node->r, index);
	return a;
}

void AVLSet(AVLNode* node, uint32_t index, rtable_entry elem){
	if(node == NULL)
		return;
	if(node->index == index){
		node->entry = elem;
		return;
	}
	if(node->index > index)
		AVLSet(node->l, index, elem);
	else
		AVLSet(node->r, index, elem);
}

