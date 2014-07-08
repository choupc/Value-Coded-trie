#include<stdlib.h>
#include<stdio.h>
#include<string.h>

#define INPUT_MODE 1

////////////////////////////////////////////////////////////////////////////////////
struct ENTRY{
	unsigned int ip;
	unsigned char len;
	unsigned char port; //nexthop
};
////////////////////////////////////////////////////////////////////////////////////
inline unsigned long long int rdtsc()
{
	unsigned long long int x;
	asm   volatile ("rdtsc" : "=A" (x));
	return x;
}
////////////////////////////////////////////////////////////////////////////////////	structure of vc-trie
struct list{	
	unsigned int port;	//nexthop , init port=-1
	unsigned int y;		//the remained prefix in node , init y=0
	unsigned int L;		//the length of remained prefix in node , init L=2
	unsigned int bin;	//number of prefix in one node , init bin=1
	struct list *left,*right;
};
typedef struct list node;
typedef node *btrie;
////////////////////////////////////////////////////////////////////////////////////	/*global variables*/
btrie root;
btrie root2;
unsigned int *query;
int num_entry=0;
int num_query=0;
struct ENTRY *table;
int N=0;	//number of nodes in VC-trie
int M=0;	//number of nodes in binary trie
unsigned long long int begin,end,total=0;
unsigned long long int *clock;
int num_node=0;//total number of nodes in the binary trie
////////////////////////////////////////////////////////////////////////////////////	creat_node
btrie create_node(){
	btrie temp;
	num_node++;
	temp=(btrie)malloc(sizeof(node));
	temp->right=NULL;
	temp->left=NULL;
	temp->port=-1;
	temp->y=0;
	temp->L=4;
	temp->bin=1;
	return temp;
}
////////////////////////////////////////////////////////////////////////////////////	read_table
void read_table(char *str,unsigned int *ip,int *len,unsigned int *nexthop){
	char tok[]="./";
	char buf[100],*str1;
	unsigned int n[4];
	sprintf(buf,"%s\0",strtok(str,tok));
	n[0]=atoi(buf);
	sprintf(buf,"%s\0",strtok(NULL,tok));
	n[1]=atoi(buf);
	sprintf(buf,"%s\0",strtok(NULL,tok));
	n[2]=atoi(buf);
	sprintf(buf,"%s\0",strtok(NULL,tok));
	n[3]=atoi(buf);
	*nexthop=n[2];
	str1=(char *)strtok(NULL,tok);
	if(str1!=NULL){
		sprintf(buf,"%s\0",str1);
		*len=atoi(buf);
	}
	else{
		if(n[1]==0&&n[2]==0&&n[3]==0)
			*len=8;
		else
			if(n[2]==0&&n[3]==0)
				*len=16;
			else
				if(n[3]==0)
					*len=24;
	}
	*ip=n[0];
	*ip<<=8;
	*ip+=n[1];
	*ip<<=8;
	*ip+=n[2];
	*ip<<=8;
	*ip+=n[3];
}
////////////////////////////////////////////////////////////////////////////////////	search for VC-trie
void search(unsigned int ip, unsigned int len){		
	int j;
	int find=0;
	unsigned int temp;
	btrie current=root;
	temp=ip;
	for(j=0;j<len;j++){
		if(current==NULL)
			break;
		if((current->y == temp)&&(current->bin == 0)){
			printf("find 0x%08x in VC-trie\n",current->y);
			find=1;
			break;
		}
		if(temp&(1<<31)){
			current=current->right;
			temp = temp << 1;
		}
		else{
			current=current->left;
			temp = temp << 1; 
		}
	}
	if(!find)	//if can't find in vc-trie, find binary trie
		search2(ip);
}
////////////////////////////////////////////////////////////////////////////////////	search for BT
void search2(unsigned int ip){		
	int j;
	btrie current=root2;
	for(j=31;j>=(-1);j--){
		if(current==NULL)
			break;
		if(current->y == ip){
			printf("find 0x%08x in BT\n",current->y);
			break;
		}
		if(ip&(1<<j)){
			current=current->right;
		}
		else{
			current=current->left; 
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////	set_table
void set_table(char *file_name){
	FILE *fp;
	int len;
	char string[100];
	unsigned int ip,nexthop;
	fp=fopen(file_name,"r");
	while(fgets(string,50,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		num_entry++;
	}
	rewind(fp);
	table=(struct ENTRY *)malloc(num_entry*sizeof(struct ENTRY));
	num_entry=0;
	while(fgets(string,50,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		table[num_entry].ip=ip;
		table[num_entry].port=nexthop;
		table[num_entry++].len=len;
	}
}
////////////////////////////////////////////////////////////////////////////////////	set_query
void set_query(char *file_name){
	FILE *fp;
	int len;
	char string[100];
	unsigned int ip,nexthop;
	fp=fopen(file_name,"r");
	while(fgets(string,50,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		num_query++;
	}
	rewind(fp);
	query=(unsigned int *)malloc(num_query*sizeof(unsigned int));
	clock=(unsigned long long int *)malloc(num_query*sizeof(unsigned long long int));
	num_query=0;
	while(fgets(string,50,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		query[num_query]=ip;
		clock[num_query++]=10000000;
		//printf("ip=0x%08x\n", ip);	//add by jungle
	}
}
////////////////////////////////////////////////////////////////////////////////////	create_VC-trie
void create_vctrie(){
	int i;
	int len_temp=0;
	int ip_temp=0;
	root=create_node();
	root2=create_node();	//for binary trie
	for(i=0;i<num_entry;i++){
		printf("i=%d-------------------------------------------------------\n",i);
		btrie ptr=root;
		int level=0;
		ip_temp=table[i].ip;
		len_temp=table[i].len;
		printf("ip=0x%08x\n",ip_temp);
		printf("len=%d\n",len_temp);
		printf("L=%d\n",ptr->L);
		while(level < table[i].len-1){
			if(len_temp > ptr->L){		//the lengh of prefix is longer than L, so the prefix can't insert to created node 
				if(ip_temp & (1<<31)){
					if(ptr->right==NULL){
						ptr->right=create_node();
						printf("Create right Node\n");
					}
					else{
						ptr=ptr->right;
						printf("Go right\n");
						ip_temp = ip_temp << 1;
						len_temp--;
						level++;
						printf("ip:0x%08x\n",ip_temp);
						printf("len:%d\n",len_temp);
						printf("level:%d\n",level);
					}
				}
				else{
					if(ptr->left==NULL){
						ptr->left=create_node();
						printf("Create left Node\n");
					}
					else{
						ptr=ptr->left;
						printf("Go left\n");
						ip_temp = ip_temp << 1;
						len_temp--;
						level++;
						printf("ip:0x%08x\n",ip_temp);
						printf("len:%d\n",len_temp);
						printf("level:%d\n",level);
					}
				}
			}
			else if(len_temp <= ptr->L){
				if(ptr->bin == 1){
					ptr->y = ip_temp;
					ptr->L = len_temp;
					ptr->port = table[i].port;
					ptr->bin--;
					printf("This node ip:0x%08x\n",ptr->y);
					printf("This node len:%d\n",ptr->L);
					break;
				}
				else{
					printf("EXCESSIVE PREFIX , ADD TO BT\n");
					add_node(table[i].ip,table[i].len,table[i].port);
					break;
				}
			}
		}
	}	
}
////////////////////////////////////////////////////////////////////////////////////	add node to binary trie
void add_node(unsigned int ip,unsigned char len,unsigned char nexthop){
	btrie ptr2=root2;
	int i;
	unsigned int temp;
	temp=ip;
	for(i=0;i<len;i++){
		if(temp&(1<<31)){
			temp = temp << 1;
			if(ptr2->right==NULL){
				ptr2->right=create_node();
			}
			ptr2=ptr2->right;
			if(i == len-1){
				ptr2->y=ip;
				ptr2->L=len;
				ptr2->port=nexthop;
				printf("Node in BT : 0x%08x , length :%d \n", ptr2->y, ptr2->L);
			}
		}
		else{
			temp = temp << 1;
			if(ptr2->left==NULL){
				ptr2->left=create_node();
			}
			ptr2=ptr2->left;
			if(i == len-1){
				ptr2->y=ip;
				ptr2->L=len;
				ptr2->port=nexthop;
				printf("Node in BT : 0x%08x , length :%d \n", ptr2->y, ptr2->L);
			}
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////	count node in VC-trie
void count_node(btrie r){
	if(r==NULL)
		return;
	count_node(r->left);
	N++;
	count_node(r->right);
}
////////////////////////////////////////////////////////////////////////////////////	count node in BT
void count_node2(btrie r){
	if(r==NULL)
		return;
	count_node2(r->left);
	M++;
	count_node2(r->right);
}
////////////////////////////////////////////////////////////////////////////////////
/*void CountClock()
{
	unsigned int i;
	unsigned int* NumCntClock = (unsigned int* )malloc(50 * sizeof(unsigned int ));
	for(i = 0; i < 50; i++) NumCntClock[i] = 0;
	unsigned long long MinClock = 10000000, MaxClock = 0;
	for(i = 0; i < num_query; i++)
	{
		if(clock[i] > MaxClock) MaxClock = clock[i];
		if(clock[i] < MinClock) MinClock = clock[i];
		if(clock[i] / 100 < 50) NumCntClock[clock[i] / 100]++;
		else NumCntClock[49]++;
	}
	printf("(MaxClock, MinClock) = (%5llu, %5llu)\n", MaxClock, MinClock);
	
	for(i = 0; i < 50; i++)
	{
		printf("%d : %d\n", (i + 1) * 100, NumCntClock[i]);
	}
	return;
}*/
////////////////////////////////////////////////////////////////////////////////////
int main(int argc,char *argv[]){
	int i,j;
	int k=0;
#ifdef INPUT_MODE

	if(argc != 2){
		printf("Please execute the file as the following way:\n");
		printf("%s  routing_table_file_name \n",argv[0]);
		exit(1);
	}
	set_query(argv[1]);
	set_table(argv[1]);
#else
	set_query("IPv4-Prefix-AS6447-2012-02-07-1-407218.txt");
	set_table("IPv4-Prefix-AS6447-2012-02-07-1-407218.txt");
#endif
////////////////////////////////////////////////////////////////////////////////////	start create trie
	printf("START CREATE\n");
	begin=rdtsc();
	create_vctrie();
	end=rdtsc();
	printf("END\n");
	printf("Avg. Inseart: %llu\n\n",(end-begin)/num_entry);
////////////////////////////////////////////////////////////////////////////////////	start search 
	printf("START SEARCH\n");
	begin=rdtsc();
	for(i=0;i<num_query;i++){
		printf("Now, search 0x%08x\n",table[i].ip);
		search(table[i].ip, table[i].len);
	}
	end=rdtsc();
	printf("END\n");
	printf("Avg. Search: %llu\n\n",(end-begin)/num_query);	
	//CountClock();
////////////////////////////////////////////////////////////////////////////////////
	count_node(root);
	count_node2(root2);
	printf("Total memory requirement: %d KB\n", (((M+N)*12)/1024));
	printf("There are %d nodes in VC-trie\n",N);
	printf("There are %d nodes in binary trie\n",M);
	return 0;
}
