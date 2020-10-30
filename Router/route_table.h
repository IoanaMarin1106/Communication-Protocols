#pragma once
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>

struct route_table_entry {
	uint32_t prefix;
	uint32_t next_hop;
	uint32_t mask;
	int interface;
	uint32_t sort_prefix;
	uint32_t sort_mask;
} __attribute__((packed));

struct arp_entry {
	uint32_t ip;
	uint8_t mac[6];
}arp_entry;
