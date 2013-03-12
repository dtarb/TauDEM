/*  TauDEM node functions that are part of D8 flow direction program.
     
  David G Tarboton, Teklu K Tesfa
  Utah State University     
  May 23, 2010
  
*/

/*  Copyright (C) 2010  David Tarboton, Utah State University

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License 
version 2, 1991 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the full GNU General Public License is included in file 
gpl.html. This is also available at:
http://www.gnu.org/copyleft/gpl.html
or from:
The Free Software Foundation, Inc., 59 Temple Place - Suite 330, 
Boston, MA  02111-1307, USA.

If you wish to use or incorporate this program (or parts of it) into 
other software that does not meet the GNU General Public License 
conditions contact the author to request permission.
David G. Tarboton  
Utah State University 
8200 Old Main Hill 
Logan, UT 84322-8200 
USA 
http://www.engineering.usu.edu/dtarb/ 
email:  dtarb@usu.edu 
*/

//  This software is distributed from http://hydrology.usu.edu/taudem/

#include "Node.h"
#include "stdlib.h"

//Merge: used to union pixels/cells/nodes together in the same set
void merge( Node *A, Node *B) {
	Node *ptr1= A;
	Node *ptr2= B;
	while( ptr1->parent != NULL ) ptr1 = ptr1->parent;
	while( ptr2->parent != NULL ) {
		 ptr2 = ptr2->parent;
	}
	if( ptr1 !=  ptr2 )
		ptr2->parent = ptr1;
}

//Collapses the unioned tree to decrease run time
void collapse( Node *A ) {
	Node *ptr = A;
	Node *tmp = A;
	Node *parent = ptr;
	while( parent->parent != NULL ) parent = parent->parent;
	while( tmp->parent != NULL ) {
		tmp = tmp->parent;
		ptr->parent = parent;
	}
}

//setAttr of whole set
void setAttr( Node *A, int newAttr ) {
	Node *ptr1 = A;
	while( ptr1->parent != NULL ) ptr1 = ptr1->parent;
	ptr1-> attr = newAttr;
}

//getAttr of whole set
int getAttr( Node *A ) {
	Node *ptr1 = A;
	while( ptr1->parent != NULL ) ptr1 = ptr1->parent;
	return ptr1->attr;
}

void setID( Node *A, int newID ) {
	Node *ptr1 = A;
	while( ptr1->parent != NULL ) ptr1 = ptr1->parent;
	ptr1-> ID = newID;
}

int getID( Node *A ) {
	Node *ptr1 = A;
	while( ptr1->parent != NULL ) ptr1 = ptr1->parent;
	return ptr1->ID;
}
