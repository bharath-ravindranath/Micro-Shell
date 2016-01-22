#include<stdio.h>
#include<stdlib.h>
#include<malloc.h>
int ht ;
struct node
{	int flag ;
	int key ;
	struct node* left ;
	struct node* right ;
} ;
typedef struct node* NODE ;

struct link
{
	NODE x;
	struct link* left;
	struct link* right;
};
typedef struct link* LINK;

NODE getnode()
{	NODE p ;
	p = (NODE) calloc(1,sizeof(struct node)) ;
		if(p == NULL)
		{	fprintf(stderr,"Out of memory\n");
			exit(EXIT_FAILURE);
		}
		return(p) ;
}

LINK getlink()
{	LINK p ;
	p = (LINK) calloc(1,sizeof(struct link)) ;
		if(p == NULL)
		{	fprintf(stderr,"Out of memory\n");
			exit(EXIT_FAILURE);
		}
		return(p) ;
}

LINK push(LINK top, NODE temp)
{
	LINK temp2;
	temp2=getlink();
	temp2->x=temp;
	temp2->right=NULL;
	if(top==NULL)
		temp2->left=NULL;
	else
	{
		temp2->left=top;
		top->right=temp2;
	}
	return(temp2);
}

NODE insert(NODE parent,int key)
{
	LINK top=NULL;
	NODE temp1=NULL ;
	NODE temp2 ;
	char a[100]={ };
	int i=-1;
	while(parent!=NULL)
	{
		temp1=parent;
		top=push(top,parent);
		if(parent->key==key)
			break;
		if(key<parent->key)
		{
			parent=parent->left;
			a[++i]='l';
		}
		else 
		{
			parent=parent->right;
			a[++i]='r';
		}
	}
	if(parent!=NULL)
	{
		while(top->left!=NULL)
			top=top->left;
		return(top->x);
	}
	temp2 = getnode() ;
	temp2->key = key ;
	temp2->flag = 0 ;
	temp2->left=temp2->right=NULL;
	ht = 1 ;
	if(temp1==NULL)
		return(temp2);
	if(key<temp1->key)
		temp1->left=temp2;
	else	
		temp1->right=temp2;
	while(top!=NULL)
	{
		parent=top->x;
		top=top->left;
		if(a[i--]=='l')
		{
		if(ht)
		{	switch(parent->flag)
			{	case -1 : // Right heavy
					  parent->flag = 0 ;
					  ht = 0 ;
					  break ;
				case 0 : // Balanced
					 parent->flag = 1 ;
					 break ;
				case 1 : // Left heavy
					 temp1 = parent->left ;
					if(temp1->flag == 1) 
					{	
						parent->left = temp1->right ;
						temp1->right = parent ;
						parent->flag = 0 ;
						parent = temp1 ;
					}
					else
					{	
						temp2 = temp1->right ;
						temp1->right = temp2->left ;
						temp2->left = temp1 ;
						parent->left = temp2->right ;
						temp2->right = parent ;
						if(temp2->flag == 1)
							parent->flag = -1 ;
						else
							parent->flag = 0 ;
						if(temp2->flag == -1)
							temp1->flag = 1 ;
						else
							temp1->flag = 0 ;
						parent = temp2 ;
					}
					if(top!=NULL)
					{
						if(a[i]=='l')
							(top->x)->left=parent;
						else
							(top->x)->right=parent;
					}
					parent->flag = 0 ;
					ht = 0 ;
			}
		}
		}
		else
		if(ht)
		{	switch(parent->flag)
			{	case 1 : // Left Heavy
					 parent->flag = 0 ;
					 ht = 0 ;
					 break ;
				case 0 : // Balanced
					 parent->flag = -1 ;
					  break ;
				case -1 : // Right Heavy
					  temp1 = parent->right ;
					 if(temp1->flag == -1)
					 {	
						parent->right = temp1->left ;
						temp1->left = parent ;
						parent->flag = 0 ;
						parent = temp1 ;
					 }
					 else
					 {	
						temp2 = temp1->left ;
						temp1->left = temp2->right ;
						temp2->right = temp1 ;
						parent->right = temp2->left ;
						temp2->left = parent ;
						if(temp2->flag == -1)
							parent->flag = 1 ;
						else
							parent->flag = 0 ;
						if(temp2->flag == 1)
							temp1->flag = -1 ;
						else
							temp1->flag = 0 ;
						parent = temp2 ;
					 }
					if(top!=NULL)
					{
						if(a[i]=='l')
							(top->x)->left=parent;
						else
							(top->x)->right=parent;
					}
					 parent->flag = 0 ;
					 ht = 0 ;
			}
		}
	}
	return(parent);
}

void display_as_tree(NODE root,int blank)
{	int i;
	if( root == NULL ) 
		return ;
	display_as_tree(root->right,blank+10) ;
	for(i=0;i<=blank;i++) printf(" ");
	printf("%d\n",root->key);
	display_as_tree(root->left,blank+10);
}
void AVL_inorder(NODE root)
{	if(root == NULL)
		return ;
	AVL_inorder(root->left) ;
	printf("%d	",root->key) ;
	AVL_inorder(root->right) ;
}

void preorderDotDump (NODE root, FILE* outputFile)
{
	if (root != NULL) 
	{
		if (root->flag == 1)
			fprintf (outputFile, "%d [label=%d,color=green];\n",\
					root->key, root->key);
		else if (root->flag == -1)
			fprintf (outputFile, "%d [label=%d,color=blue];\n",\
					root->key, root->key);
		else
			fprintf (outputFile, "%d [label=%d,color=black];\n",\
					root->key, root->key);
		if ((root->left)!= NULL) 
		         fprintf (outputFile, "%d -> %d ;\n", root->key, (root->left)->key);
		
        	if ((root->right) != NULL)
            		fprintf (outputFile, "%d -> %d ;\n", root->key, (root->right)->key);
		
        	preorderDotDump (root->left, outputFile);
        	preorderDotDump (root->right, outputFile);
    	}
}

void dotDump(NODE root,  FILE *outFile)
{	
	fprintf (outFile, "digraph avl {\n");
	preorderDotDump (root, outFile);
    	fprintf (outFile, "}\n");
}

NODE find(NODE root, int key)
{
	while(root!=NULL && root->key!=key)
		root=(key < root->key)? root->left : root->right;
	return(root);
}

int main()
{	int n,key,i;	
	NODE root = NULL;
	FILE *pipe;
	FILE *outputFile;
	system(" rm AVL.dot AVL.ps 2>/dev/null ") ;
	printf("Number of elements ? \n");
	scanf("%d",&n);
	srandom(n);
	for(i=0;i<n;i++)
	{	key = random() % (100*n) ;
		root = insert(root,key);
		outputFile = fopen("AVL.dot","a") ;
		dotDump (root, outputFile);
		fclose (outputFile);
		
	}
 				 pipe=popen("dot -Tps AVL.dot -o AVL.ps","w") ;
				 pclose(pipe) ;	
				 pipe=popen("evince AVL.ps","r") ; 
			       	 pclose(pipe) ;
				return(0) ;
				
}
