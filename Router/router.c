#include "skel.h"
#include "route_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/if_ether.h>
#include "queue.h"

#define MAX_SIZE 9999999

struct arp_entry *arp_table;
int arp_table_size;

/* 
	Functie care converteste o adresa IP din format big-endian
	in format little-endian.
*/
uint32_t big_to_litlle(char *str) {
	uint32_t first_part, second_part, third_part, fourth_part;
	uint32_t result = 0;

	sscanf(str, "%u.%u.%u.%u", 
			&first_part, &second_part, 
			&third_part, &fourth_part);

	result = (first_part << 24) + (second_part << 16) + 
			(third_part << 8) + fourth_part;
	return result;
}

/*
	Functie care citeste tabela de rutare.
*/
int read_route_table(FILE *file, 
					struct route_table_entry **rtable) {

	char chunk[128];
	int rtable_size = 0;

	size_t len = sizeof(chunk);
	char *line = malloc(len);

	if (line == NULL) {
		perror("Unable to alocate memory for the line buffer");
		exit(1);
	}

	line[0] = '\0';

	while (fgets(chunk, sizeof(chunk), file) != NULL) {
		size_t len_used = strlen(line);
		size_t chunk_used = strlen(chunk);

		if (len - len_used < chunk_used) {
			len *= 2;
			if ((line = realloc(line, len)) == NULL) {
				perror("Unable to realloc memory for the line buffer");
				free(line);
				exit(1);
			}
		}

		strncpy(line + len_used, chunk, len - len_used);
		len_used += chunk_used;

		if (line[len_used - 1] == '\n') {

			char str_prefix[16];
			char str_next_hop[16];
			char str_mask[16];
			int interface;
			char sort_prefix[16];
			char sort_mask[16];

			sscanf(line, "%s %s %s %d", str_prefix, str_next_hop, 
					str_mask, &interface);

			memcpy(sort_prefix, str_prefix, 16);
			memcpy(sort_mask, str_mask, 16);

			/* Se seteaza campurile intrarii respective in tabela de rutare */
			(*rtable)[rtable_size].prefix = inet_addr(str_prefix);
    		(*rtable)[rtable_size].next_hop = inet_addr(str_next_hop);
    		(*rtable)[rtable_size].mask = inet_addr(str_mask);
    		(*rtable)[rtable_size].interface = interface;
    		(*rtable)[rtable_size].sort_prefix = big_to_litlle(sort_prefix);
    		(*rtable)[rtable_size].sort_mask = big_to_litlle(sort_mask);

			line[0] = '\0';
    		rtable_size += 1;
		}
	}

	/* 
		Se realoca tabela de rutare la dimensiunea sa fixa, pentru a nu 
		folosi memorie in plus.
	*/
	*rtable = realloc(*rtable, sizeof(struct route_table_entry) * rtable_size);
	if (*rtable == NULL) {
        perror("Unable to realloc memory for the route table");
        exit(1);
    }

	fclose(file);
	free(line);
	return rtable_size;
}

/*
	Functia care calculeaza checksum, functie preluata din
	laboratorul 4, laboratorul de forwarding.
*/
uint16_t ip_checksum(void* vdata, size_t length) {
	// Cast the data pointer to one that can be indexed.
	char* data=(char*)vdata;

	// Initialise the accumulator.
	uint64_t acc=0xffff;

	// Handle any partial block at the start of the data.
	unsigned int offset=((uintptr_t)data)&3;
	if (offset) {
		size_t count=4-offset;
		if (count>length) count=length;
		uint32_t word=0;
		memcpy(offset+(char*)&word,data,count);
		acc+=ntohl(word);
		data+=count;
		length-=count;
	}

	// Handle any complete 32-bit blocks.
	char* data_end=data+(length&~3);
	while (data!=data_end) {
		uint32_t word;
		memcpy(&word,data,4);
		acc+=ntohl(word);
		data+=4;
	}
	length&=3;

	// Handle any partial block at the end of the data.
	if (length) {
		uint32_t word=0;
		memcpy(&word,data,length);
		acc+=ntohl(word);
	}

	// Handle deferred carries.
	acc=(acc&0xffffffff)+(acc>>32);
	while (acc>>16) {
		acc=(acc&0xffff)+(acc>>16);
	}

	// If the data began at an odd byte address
	// then reverse the byte order to compensate.
	if (offset&1) {
		acc=((acc&0xff00)>>8)|((acc&0x00ff)<<8);
	}

	// Return the checksum in network byte order.
	return htons(~acc);
}

/*
	Funcie care cauta in tabela ARP, folosind adresa IP data ca parametru, 
	intrarea corespunzatoarea acesteia, si intoarce un pointer catre aceasta.
*/
struct arp_entry *get_arp_entry(__u32 ip) {
	for (int i = 0; i < arp_table_size; i++ ){
		if (arp_table[i].ip == ip) {
			return &(arp_table[i]);
		}
	}
    return NULL;
}

/*
	Functie in care se completeaza campurile specifice unui pachet,
	campurile header-ului ethernet, campurile header-ului IP si 
	campurile header-ului ICMP, iar tipul pachetului de tip ICMP va fi 
	setat cu valoarea primita ca parametru, in functie de ce tip de pachet
	este. Dupa care routerul trimite pachetul pe aceeasi interfata ca 
	cea a pachetului pe care l-a primit.
*/
void send_packet_reply(packet *m, uint8_t type) {
	packet reply;

	/* Se seteaza campurile pachetului. */
	reply.len = sizeof(struct ether_header) + 
				sizeof(struct iphdr) + 
				sizeof(struct icmphdr); 

	reply.interface = m->interface;

	/* Se extrage ethernet header pentru ambele pachete. */
	struct ether_header *ether_header_reply = (struct ether_header*)reply.payload;
	struct ether_header *ether_header = (struct ether_header*)m->payload;

	ether_header_reply->ether_type = ether_header->ether_type;
	memcpy(ether_header_reply->ether_dhost, ether_header->ether_shost, 6);
	memcpy(ether_header_reply->ether_shost, ether_header->ether_dhost, 6);

	/* Se extrage IP header pentru ambele pachete. */
	struct iphdr* ip_hdr_reply = (struct iphdr*)(reply.payload + 
												sizeof(struct ether_header));
	struct iphdr* ip_hdr = (struct iphdr*)(m->payload + 
							sizeof(struct ether_header));

	ip_hdr_reply->tos = 0;
	ip_hdr_reply->version = 4;
	ip_hdr_reply->ihl = 5;
	ip_hdr_reply->tot_len = htons(sizeof(struct iphdr) + 
							sizeof(struct icmphdr));
	ip_hdr_reply->protocol = IPPROTO_ICMP;
	ip_hdr_reply->id = htons(getpid());
	ip_hdr_reply->frag_off = 0;
	ip_hdr_reply->ttl = 64;
	ip_hdr_reply->saddr = ip_hdr->daddr;
	ip_hdr_reply->daddr = ip_hdr->saddr;
	ip_hdr_reply->check = 0;
	ip_hdr_reply->check = ip_checksum(ip_hdr_reply, sizeof(struct iphdr));

	/* Se extrage ICMP header pentru pachetul pe care dorim sa-l trimitem. */
	struct icmphdr* icmp_hdr_reply = (struct icmphdr*)(reply.payload +
												sizeof(struct iphdr) +
												sizeof(struct ether_header));

	icmp_hdr_reply->type = type;
	icmp_hdr_reply->code = 0;
	icmp_hdr_reply->un.echo.id = htons(getpid());
	icmp_hdr_reply->un.echo.sequence = htons(64);
	icmp_hdr_reply->checksum = 0;
	icmp_hdr_reply->checksum = ip_checksum(icmp_hdr_reply, 
											sizeof(struct icmphdr));

	/* Se trimite pachetul. */
	send_packet(m->interface, &reply);
}

/*
	Functie data ca parametru functiei "qsort(...)", functie ce decide 
	ordinea in care se va face sortarea tabelei de rutare: crescator in 
	functie de prefixul calculat in format little-endian si in cazul in 
	care prefixele sunt egale atunci se sorteaza crescator in functie de
	masca calculata in format little-endian.
*/
int comparator(const void* a, const void* b) {
    if (((struct route_table_entry*)a)->sort_prefix == ((struct route_table_entry*)b)->sort_prefix && 
    	((struct route_table_entry*)a)->sort_mask > ((struct route_table_entry*)b)->sort_mask)
        return 1;

    if (((struct route_table_entry*)a)->sort_prefix == ((struct route_table_entry*)b)->sort_prefix && 
    	((struct route_table_entry*)a)->sort_mask < ((struct route_table_entry*)b)->sort_mask)
        return -1;

     if (((struct route_table_entry*)a)->sort_prefix > ((struct route_table_entry*)b)->sort_prefix)
        return 1;

    if (((struct route_table_entry*)a)->sort_prefix <= ((struct route_table_entry*)b)->sort_prefix)
        return -1;

    return 1;
}

/*
	Functie care reprezinta cautarea binara in tabela de rutare, in O(log n),
	ruta cea mai specifica pentru adresa IP data ca parametru.
*/
struct route_table_entry* binary_search(struct route_table_entry* rtable,
										int rtable_size, 
										int left, int right, 
										uint32_t dest_ip) { 
    if (right >= left) { 
        int mid = left + (right - left) / 2; 
  		
        if ((dest_ip & rtable[mid].sort_mask) < rtable[mid].sort_prefix) {
        	return binary_search(rtable, rtable_size, 
        						left, mid - 1, (dest_ip)); 
        }
  
        if ((dest_ip & rtable[mid].sort_mask) > rtable[mid].sort_prefix) {
            return binary_search(rtable, rtable_size, 
            					mid + 1, right, dest_ip);
        }

        if ((dest_ip & rtable[mid].sort_mask) == rtable[mid].sort_prefix) {
        	/* 
        		Cautam prefixul cu cea mai mare masca, cautarea facandu-se pana
        		cand nu se mai indeplineste conditia.
        	*/
        	while ((dest_ip & rtable[mid + 1].sort_mask) == rtable[mid + 1].sort_prefix) {
        		mid += 1;
        		if (mid == rtable_size) {
        			break;
        		}
        	}
        	return &rtable[mid];
        } 
    } 
    return NULL; 
} 

/*
	Functie care returneaza cea mai specifica ruta din tabela de rutare 
	pentru adresa IP data ca parametru.
*/
struct route_table_entry* get_best_route(__u32 dest_ip,
										struct route_table_entry *rtable,
										int rtable_size) {

	struct route_table_entry* best_route = binary_search(rtable, 
														rtable_size, 0, 
														rtable_size, 
														ntohl(dest_ip)); 
	return best_route;
}

/*
	Functie folosita pentru trimiterea unui pachet de tip ARPOP_REPLY.	
*/
void send_packet_arp_reply(packet *m, int ar_op) {
	packet reply;

	/* Se seteaza campurile pachetului. */
	reply.len = sizeof(struct ether_header) + sizeof(struct ether_arp);
	reply.interface = m->interface;

	/* Se extrage ethernet header pentru ambele pachete. */
	struct ether_header *eth_hdr = (struct ether_header*)m->payload;
	struct ether_header *eth_hdr_reply = (struct ether_header*)reply.payload;

	eth_hdr_reply->ether_type = eth_hdr->ether_type;
	memcpy(eth_hdr_reply->ether_dhost, eth_hdr->ether_shost, 6);
	get_interface_mac(m->interface, eth_hdr_reply->ether_shost);

	/* Se extrage ARP header pentru ambele pachete. */
	struct ether_arp *eth_arp_reply = (struct ether_arp*)(reply.payload +
										 sizeof(struct ether_header));
	struct ether_arp *eth_arp = (struct ether_arp*)(m->payload + 
								sizeof(struct ether_header));

	eth_arp_reply->ea_hdr.ar_hrd = eth_arp->ea_hdr.ar_hrd;
	eth_arp_reply->ea_hdr.ar_pro = eth_arp->ea_hdr.ar_pro;
	eth_arp_reply->ea_hdr.ar_hln = 6;
	eth_arp_reply->ea_hdr.ar_pln = 4;
	eth_arp_reply->ea_hdr.ar_op = htons(ar_op);

	get_interface_mac(m->interface, eth_arp_reply->arp_sha);
	memcpy(eth_arp_reply->arp_tha, eth_arp->arp_sha, ETH_ALEN);
	memcpy(eth_arp_reply->arp_spa, eth_arp->arp_tpa, 4);
	memcpy(eth_arp_reply->arp_tpa, eth_arp->arp_spa, 4);

	/* Se trimite pachetul. */
	send_packet(m->interface, &reply);
}

/*
	Functie care updateaza tabela ARP cu noua intrare ce contine
	o noua adresa MAC si un noua adresa IP corespunzatoare 
	adresei MAC.
*/
void update_arp_table(packet *m) {
	struct ether_arp *eth_arp = (struct ether_arp*)(m->payload + 
								+ sizeof(struct ether_header));

	/* Setarea adresei IP din noua intrare din tabela ARP. */
	arp_table[arp_table_size].ip = eth_arp->arp_spa[0] + 
								(eth_arp->arp_spa[1] << 8) +
								(eth_arp->arp_spa[2] << 16) + 
								(eth_arp->arp_spa[3] << 24);

	/* Setarea adresei MAC din noua intrare din tabela ARP. */
	memcpy(arp_table[arp_table_size].mac, eth_arp->arp_sha, 6);

	/* Se mareste dimensiunea tabelei ARP. */
	arp_table_size += 1;
}

/*
	Functie care verifica daca adresa IP a pachetului dat ca parametru 
	corespunde cu cea a ultimei intrari in tabela ARP.
*/
int check_packet(packet *p) {
	struct arp_entry my_entry = arp_table[arp_table_size - 1]; 

	struct iphdr* ip_hdr = (struct iphdr*)(p->payload + 
							sizeof(struct ether_header));

	if (ip_hdr->daddr == my_entry.ip) {
		return 1;
	}
	return 0;
}

/*
	Functie care verifica daca, avand o noua intrare in tabela ARP, 
	putem sa trimitem un pachet din coada pachetelor de transmitere.
*/
void verify_forwarding_packets(queue q, 
								struct route_table_entry *rtable, 
								int rtable_size) {
	queue q_aux = queue_create();
	struct arp_entry my_entry = arp_table[arp_table_size - 1];

	/* 
		Cat timp coada pachetelor de transmitere este nevida, 
		extragem pachete pentru care verificam conditia.
	*/
	while (!queue_empty(q)) {
		packet *p = queue_deq(q);

		struct ether_header *eth_hdr = (struct ether_header*)p->payload;
		struct iphdr* ip_hdr = (struct iphdr*)(p->payload + 
									sizeof(struct ether_header));
		if (check_packet(p) == 1) {
			/* Setam adresa MAC pachetului respectiv, o cunoastem.*/
			memcpy(eth_hdr->ether_dhost, my_entry.mac, 6);

			/* Trimitem pachetul.*/
			send_packet(get_best_route(ip_hdr->daddr, 
									rtable, rtable_size)->interface, p);
		} else {
			/* 
				Altfel, bagam pachetul intr-o coada auxiliara, asteptand
				sa primim o adresa MAC si pentru aceste pachete.
			*/
			queue_enq(q_aux, p);
		}
	}

	/* Refacem coada initiala. */
	while (!queue_empty(q_aux)) {
		packet *p = queue_deq(q_aux);
		queue_enq(q, p);
	}

	return;
}

/*
	Functie care este folosita pentru trimiterea unui pachet de tip ARPOP_REQUEST.
*/
void send_arp_request(uint32_t daddr, struct route_table_entry *route_table,
					int rtable_size, int ar_op) {
	packet request;

	/* Se seteaza campurile pachetului. */
	request.len = sizeof(struct ether_header) + sizeof(struct ether_arp);
	struct route_table_entry *best_route = get_best_route(daddr, 
														route_table,
														rtable_size);
	request.interface = best_route->interface;

	/* Se extrage ethernet header pentru pachetul nostru. */
	struct ether_header *eth_hdr_request = (struct ether_header*)request.payload;
	eth_hdr_request->ether_type = htons(ETHERTYPE_ARP);

	for (int i = 0; i < 6; i++) {
		eth_hdr_request->ether_dhost[i] = 255;
	}
	get_interface_mac(best_route->interface, eth_hdr_request->ether_shost);

	/* Se extrage ARP header pentru ambele pachete. */
	struct ether_arp *eth_arp_request = (struct ether_arp*)(request.payload
										+ sizeof(struct ether_header));
	
	eth_arp_request->ea_hdr.ar_hrd = htons(1);
	eth_arp_request->ea_hdr.ar_pro = htons(ETH_P_IP);
	eth_arp_request->ea_hdr.ar_hln = 6;
	eth_arp_request->ea_hdr.ar_pln = 4;
	eth_arp_request->ea_hdr.ar_op = ar_op;

	get_interface_mac(best_route->interface, eth_arp_request->arp_sha);
	uint32_t sender_protocol_address = inet_addr(get_interface_ip(
												best_route->interface));
    memcpy(eth_arp_request->arp_spa, &sender_protocol_address, 4);
    memcpy(eth_arp_request->arp_tpa, &daddr, 4);

    /* Se trimite pachetul.*/
	send_packet(request.interface, &request);
}

/*
	Functie care adauga un pachet in coada de transmitere pachete.
*/
void put_packet_in_queue(queue q, packet m) {
	packet *aux_packet = (packet*)malloc(sizeof(packet));

	aux_packet->len = m.len;
	aux_packet->interface = m.interface;
	memcpy(aux_packet->payload, m.payload, MAX_LEN);

	queue_enq(q, aux_packet);
	return;
}

int main(int argc, char *argv[])
{
	packet m;
	int rc;

	init();

	FILE *input_rtable_file = fopen("rtable.txt", "r");
	if (input_rtable_file == NULL) {
		perror("Unable to open the input file.");
		exit(1);
	}

	/* Alocare spatiu pentru tabela ARP. */
	arp_table = (struct arp_entry*)malloc(sizeof(struct arp_entry) * 100);
	if (arp_table == NULL) {
		perror("Unable to alocate memory for arp table");
		exit(1);
	}
	arp_table_size = 0;

	/* Alocare spatiu pentru tabela de rutare.*/
	int current_size = MAX_SIZE;
	struct route_table_entry *rtable;
	rtable = malloc(sizeof(struct route_table_entry) * current_size);
	if (rtable == NULL) {
		perror("Unable to alocate memory for route table.");
		exit(1);
	}
	int rtable_size = read_route_table(input_rtable_file, &rtable);

	/* Sortarea tabelei de rutare, folositoare pentru cautarea binara.*/
	qsort(rtable, rtable_size, sizeof(struct route_table_entry), comparator);

	/* Initializarea cozii de trasnmitere de pachete. */
	queue q = queue_create();

	while (1) {
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");

		/* Se extrage ethernet header pentru pachet.*/
		struct ether_header *ether_header = (struct ether_header*)m.payload;

		/* Se verifica daca e un pachet de tipul IP.*/
		if (ether_header->ether_type == htons(ETHERTYPE_IP)) {

			/* Se extrage IP header pentru pachet.*/
			struct iphdr* ip_hdr = (struct iphdr*)(m.payload + 
												sizeof(struct ether_header));

			/* Se verifica daca e un pachet destinat router-ului.*/
			if (inet_addr(get_interface_ip(m.interface)) == ip_hdr->daddr) {

				/* Se verifica daca e un pachet de tipul ICMP.*/
				if (ip_hdr->protocol == IPPROTO_ICMP) {
					struct icmphdr* icmp_hdr = (struct icmphdr*)(m.payload + 
												sizeof(struct iphdr) + 
												sizeof(struct ether_header));

					/* 
						Se verifica daca e un pachet de tip ICMP_ECHO reply,
						daca da trimitem un pachet de tip ICMP_ECHO_REQUEST.
					*/
					if (icmp_hdr->type == ICMP_ECHO) {
						send_packet_reply(&m, ICMP_ECHOREPLY);
						continue;
					}
				}
			}

			/* Se verifica ttl-ul pachetului.*/
			if (ip_hdr->ttl <= 1) {
				send_packet_reply(&m, ICMP_TIME_EXCEEDED);
				continue;
			}

			/* Se verifica daca are un checksum gresit. */
			if (ip_checksum(ip_hdr, sizeof(struct iphdr)) != 0) {
				continue;
			}

			/* Cautam cea mai specifica ruta din tabela de rutare.*/
			struct route_table_entry *best_route = get_best_route(ip_hdr->daddr, 
																rtable, 
																rtable_size);
			if (best_route == NULL) {
				send_packet_reply(&m, ICMP_DEST_UNREACH);
				continue;
			}

			ip_hdr->ttl -= 1;
			ip_hdr->check = 0;
			ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));

			/*
				Se verifica daca este cunoscuta local adresa MAC a
				pachetului primit folosind "get_arp_entry(...)".
			*/
			struct arp_entry *my_entry = get_arp_entry(ip_hdr->daddr);

			if (my_entry == NULL) {
				send_arp_request(ip_hdr->daddr, 
								rtable, 
								rtable_size, 
								htons(ARPOP_REQUEST));

				put_packet_in_queue(q, m);
				continue;
			}

			memcpy(ether_header->ether_dhost, my_entry->mac, 6);

			/* Se trimite pachetul.*/
			send_packet(best_route->interface, &m);
			continue;

		} else if (ether_header->ether_type == htons(ETHERTYPE_ARP)) {

			/* Se extrage ARP header-ul pentru pachet.*/
			struct ether_arp* arp_hdr = (struct ether_arp*)(m.payload +
											 sizeof(struct ether_header));

			/* Se verifica daca e un pachet de tipul ARPO_REQUEST.*/
			if (arp_hdr->ea_hdr.ar_op == htons(ARPOP_REQUEST)) {
				send_packet_arp_reply(&m, ARPOP_REPLY);
				continue;
			}

			/* Se verifica daca e un pachet de tipul ARPO_REPLY.*/
			if(arp_hdr->ea_hdr.ar_op == htons(ARPOP_REPLY)) {
				update_arp_table(&m);
				verify_forwarding_packets(q, rtable, rtable_size);
				continue;
			}
			continue;
		}
	}
}