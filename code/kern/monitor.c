// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/pmap.h>
#include <kern/util.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
        const char *name;
        const char *desc;
        // return -1 to force monitor to exit
        int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
    { "help", "Display this list of commands", mon_help },
    { "kerninfo", "Display information about the kernel", mon_kerninfo },
    { "backtrace", "Display the backtrace of function call", mon_backtrace },
    { "showmappings", "Display the virtual address mappings of current address space", mon_showmappings },
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
    uint32_t *ebp = (uint32_t *)read_ebp();
    uint32_t eip = ebp[1];
    struct Eipdebuginfo info;

    cprintf("Stack backtrace:\n");
    while (ebp != NULL)
    {
        cprintf("  ebp %08x  eip %08x  args ", ebp, eip);
        for (int i = 0; i < 5; i++)
        {
            cprintf("%08x ", ebp[2 + i]);
        }
        cprintf("\n");

        debuginfo_eip(eip, &info);
        cprintf("         %s:%d: ", info.eip_file, info.eip_line);
		for (int i = 0; i < info.eip_fn_namelen; i++) {
            cprintf("%c", info.eip_fn_name[i]);
		}
		cprintf("+%d\n", eip - info.eip_fn_addr);

        ebp = (uint32_t *)ebp[0];
        eip = ebp[1];
    }
    return 0;
}

int
mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
    uint32_t  start_addr;
    uint32_t  end_addr;
    pte_t    *entry;

    if (argc != 3 || !read_hex_from_str(argv[1], &start_addr) || !read_hex_from_str(argv[2], &end_addr))
    {
        cprintf("mon_showmappings: Invalid arguments!\n");
        return -1;
    }

    cprintf("start:%08x end:%08x\n", start_addr, end_addr);
    if (((start_addr % 0x1000) != 0) || ((end_addr % 0x1000) != 0))
    {
        cprintf("mon_showmappings: Address doesn't align with page!\n");
        return -1;
    }

    if (start_addr > end_addr)
    {
        cprintf("mon_showmappings: Start address greater than end address!\n");
        return -1;
    }

    cprintf("mon_showmappings: mappings from [0x%08x, 0x%08x]\n", start_addr, end_addr);
    cprintf("|  Virtual   ||  Physical  |\n");
    for (uint32_t addr = start_addr; addr <= end_addr; addr += 0x1000)
    {
        struct PageInfo *page = page_lookup(kern_pgdir, (void *)addr, &entry);
        if (page != NULL && ((*entry) & PTE_P))
        {
            cprintf("| 0x%08x -> 0x%08x\n", addr, page2pa(page));
        }
        else
        {
            cprintf("| 0x%08x -> unmapped\n");
        }
    }

    return 0;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
