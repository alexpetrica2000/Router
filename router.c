#include <queue.h>
#include "skel.h"
#define BUFLEN 16

int size;
struct arp_entry* arp_entries;
AVLTree *rtable;

// functie ce compara uint32 folosita de AVL
uint32_t compare(uint32_t a, uint32_t b) {
	if(a > b)
		return 1;
	if(a < b) 
		return -1;
	return 0;
}
// functie pentru aflarea best entry - routului din rtable
// practcic incepe de la masca maxima /32 si descreste /31,/30..0 prin shiftari
struct rtable_entry *get_best_entry(uint32_t ip) {

	uint32_t mask = 0xffffffff; // masca /32
	struct rtable_entry* best = NULL;
	// se parcurg cele 32 de masti in ordine descresc, la prima gasita e max
	for(int i = 0; i < 32; i++) {
		// se cauta in avl, campul index reprezinta indicele 
		// de comparare in avl, in cazul nostru prefixul, deci caut ip & mask
		best = search(rtable->root, ip & mask);
	
		if(best != NULL)
			return best;
		mask = mask >> 1;
	}

	return best;
}
// functie ce afla arp entry-ul, parcurge tabela arp pana gaseste ip-ul pentru
// pentru a-i folosi macul
struct arp_entry *get_arp_entry(uint32_t  ip) {
	for(int i = 0; i < size; i++){
		if(arp_entries[i].ip == ip){
			return &arp_entries[i];
		}
	}
    return NULL;
}

int main(int argc, char *argv[])
{
	packet m;
	int rc, fd;
	int maxsize = 10;
	// creez avl, trimit functia de compare (intre care compara elementele)
	// creez arp entries si coada
	rtable = avlCreateTree(compare);
	arp_entries = malloc(maxsize*sizeof(struct arp_entry));
	queue q = queue_create();
	queue q2;

	// buferele in care citesc, marime standard 16
	char b1[BUFLEN];
	char b2[BUFLEN];
	char b3[BUFLEN];

	size = 0;
	uint32_t myip;
	FILE* file = fopen(argv[1],"r");

	// aici fac citirea din fisier rtable in structura mea AVL
	// practic aici are loc parsarea rtable
	while(fscanf(file,"%s %s %s %d",b1,b2,b3,&fd) != EOF) {

		rtable_entry entry;
		inet_aton(b1, &entry.prefix);
		inet_aton(b2, &entry.nexthop);
		inet_aton(b3, &entry.mask);
		entry.interface = fd;
		avlInsert(rtable, entry, entry.prefix);
	}
	fclose(file);

	init(argc - 2, argv + 2);
    while (1) {
        rc = get_packet(&m);
        DIE(rc < 0, "get_message");
        struct ether_header *eth_hdr = (struct ether_header *)m.payload;

        // testez daca am pachit de tip arp
        if (eth_hdr->ether_type == htons(0x0806)) {
        	 struct arp_header* arp_hdr = parse_arp(m.payload);

        	 if (arp_hdr != NULL) {  // verific daca parsarea s-a efectuat
	        	inet_aton(get_interface_ip(m.interface),&myip); // pun in myip ip-ul de pe interfata respectiva

	        	if (arp_hdr->tpa == myip) { // verific daca este destinat pentru mine
		        	if (arp_hdr->op == htons(ARPOP_REQUEST)) { // primesc req
		        		// actualizez pachetul arp pentru a fi trimis arp reply
		        		// interschimb dhost cu shost si in loc de broadcast pun arp de pe interfata
		        		// pe care o am cu hostul care a trimis req + interschimb ip-uri sursa dest
		   				memcpy(eth_hdr->ether_dhost,eth_hdr->ether_shost,sizeof(eth_hdr->ether_shost));
		   				get_interface_mac(m.interface, eth_hdr->ether_shost);
		   				send_arp(arp_hdr->spa, arp_hdr->tpa, eth_hdr, m.interface, htons(ARPOP_REPLY));

		   				continue;
		        	}
		        else if (arp_hdr->op == htons(ARPOP_REPLY)){ // primesc reply + update arp table + trimite packet din queue
			        	arp_entries[size].ip = arp_hdr->spa;
		        		memcpy(arp_entries[size].mac, arp_hdr->sha, sizeof(arp_hdr->sha));
						size++; 

						// daca size ajunge la nivelul lui max, realloc cu o dimensiune dubla
						if(size == maxsize){
							maxsize = maxsize * 2;
							 arp_entries = realloc(arp_entries, sizeof(struct arp_entry) * maxsize);
						}

						// daca am primit reply verific ce pachete din coada trebuiesc trimise
						while (!queue_empty(q)) {

							q2 = queue_create(); // coada in care vor fi puse pachete ce nu au primit inca adresa mac dest
							
							// scot pachetul si fac modificarile necesare pentru a-l trimite mai departe
							// caut iar best entryul pentru safety, caut in tabela arp potrivirea si
							// reglez shostul si dhostul din ether header
							packet *t = queue_deq(q);
							eth_hdr = (struct ether_header *)t->payload;
							struct iphdr *ip_hdr = (struct iphdr *)(t->payload + sizeof(struct ether_header));
							rtable_entry *best_entry = get_best_entry(ip_hdr->daddr);

							struct arp_entry *best_arp_entry = NULL; 

							best_arp_entry = get_arp_entry(best_entry->nexthop);

							if (best_arp_entry == NULL) {
								packet z = *t;
								queue_enq(q2, &z);
							}
							get_interface_mac(best_entry->interface, eth_hdr->ether_shost);
							eth_hdr->ether_type = htons(0x0800);

							memcpy(eth_hdr->ether_dhost, best_arp_entry->mac, sizeof(best_arp_entry->mac));
							send_packet(best_entry->interface, t);

						}
						q = q2; // interschimb cozile (q e oricum vida)
					continue;
		        	}
		        }
		     }
		}
        

        if (eth_hdr->ether_type == htons(0x0800)) { // caz in care am pachet ip 

        	struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
        	struct icmphdr * icmp_hdr = parse_icmp(m.payload);
        	inet_aton(get_interface_ip(m.interface),&myip); // adresa mea ip
   

        	if (ip_hdr->daddr == myip && icmp_hdr != NULL && icmp_hdr->type == 8) { //caz in care am primit icmp echo request
        			send_icmp(ip_hdr->saddr, ip_hdr->daddr, eth_hdr->ether_dhost, 
        				eth_hdr->ether_shost, 0, 0, m.interface, icmp_hdr->un.echo.id, icmp_hdr->un.echo.sequence);
        			continue;
        	}
        	if (ip_hdr->ttl <= 1) { // Daca ttl <= 1 trimit icmp time exceeded
        		send_icmp_error(ip_hdr->saddr, ip_hdr->daddr, eth_hdr->ether_dhost, 
        			eth_hdr->ether_shost, 11, 0, m.interface);
        		continue;

        	}
        	if (ip_checksum(ip_hdr, sizeof(struct iphdr))) { // daca checksum e gresit drop pachet
				continue;	
        	}
        	// actualieze ttl si checksum
			ip_hdr->ttl = ip_hdr->ttl - 1;
			ip_hdr->check = 0;
			ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));

			// aflu best entry din tabela de rutare, daca e null trimit icmp
			rtable_entry *best_entry = get_best_entry(ip_hdr->daddr);

			if (best_entry == NULL) { // trimite icmp sursei destination unreachable
				send_icmp_error(ip_hdr->saddr, ip_hdr->daddr, eth_hdr->ether_dhost, eth_hdr->ether_shost, 3, 0, m.interface);
				continue;
			}

			struct arp_entry *best_arp_entry = NULL; 

			// aflu  matchul ip - mac din tabela arp 
			best_arp_entry = get_arp_entry(best_entry->nexthop);

			
			// daca nu am gasit match in tabela arp trimit arp request pt mac si pun pachet in coada
			if(best_arp_entry == NULL) {

				// pun in coada o copie a pachetului original
				packet z = m;
				queue_enq(q,&z);
				
				//setez macul pt broadcast
				hwaddr_aton("ff:ff:ff:ff:ff:ff",eth_hdr->ether_dhost);
				
				// setez ip-ul de pe interfata pe care ies + pun macul pentru interfata
				// respectiva in ether shost
				inet_aton(get_interface_ip(best_entry->interface), &myip);
				get_interface_mac(best_entry->interface, eth_hdr->ether_shost);
				// ether_type tip arp
				eth_hdr->ether_type = htons(0x0806);
				send_arp(best_entry->nexthop, myip , eth_hdr, best_entry->interface, htons(ARPOP_REQUEST));
			}
			else {
				// altfel daca cunosc macul destinatie ( best arp entry nu e NULL)
				// fac modificarile in ether shost cu macul interfetei pe care ies
				// si dhost macul destinatiei si trimit pachetul
				get_interface_mac(best_entry->interface, eth_hdr->ether_shost);
				memcpy(eth_hdr->ether_dhost, best_arp_entry->mac, sizeof(best_arp_entry->mac));
				send_packet(best_entry->interface, &m);
			}
        }
    }
    free(arp_entries);
	return 0;
}


