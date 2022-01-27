/**
 *  Depending on which allocator is being used, the corresponding namespace will be defined
 * The 'malloc' namespace is currently always defined, as it is assumed that the glibc allocator
 * is always present, even if not used
 */

/** The type of allocator used: currently can be malloc or jemalloc */
export const allocator: string;

export interface HeapUsage {
    used: number;
    free: number;
}
/** Allocator-independent function to get basic heap usage statistics */
export function getHeapUsage(): HeapUsage;

export namespace malloc {
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
    export function info(): string;

    /** Binding for malloc_stats() */
    export function stats(): void;

    /** Binding for malloc_trim() */
    export function trim(pad: number): number;
}

export namespace jemalloc {
    /** All ctlGetXXX() functions map to the mallctl() interface of jemalloc,
     * in read mode, for the corresponding type
     */
    export function ctlGetSize(name: string): number;
    export function ctlGetString(name: string): string;
    export function ctlGetBool(name: string): boolean;
    export function ctlGetU64(name: string): number;
    export function flushThreadCache(): void;
}
