/*
 * Structure returned by mallinfo() / mallinfo2()
 * For more information, see manpage of mallinfo()
 */
export interface mallinfo {
    arena: number;    /* Total size of heap in bytes: sum of used and free space (uordblks + fordblks) */
    ordblks: number;
    smblks: number;
    hblks: number;
    hblkhd: number;
    usmblks: number;
    fsmblks: number;
    uordblks: number; /* Total bytes in use */
    fordblks: number; /* Total bytes free */
    keepcost: number;
}

/**
 * Binding for mallinfo2()
 * NOTE: in glibc < 2.33 mallinfo2() is not present. In this case, this call maps to the deprecated
 * mallinfo() function, which differs by having the values in the returned struct as 32-bit integers,
 * which means that they may wrap.
 */
export function mallinfo2(): mallinfo;

/** Binding for malloc_info() */
export function malloc_info(): string;

/** Binding for malloc_stats() */
export function malloc_stats(): void;

/** Binding for malloc_trim() */
export function malloc_trim(pad: number): number;
