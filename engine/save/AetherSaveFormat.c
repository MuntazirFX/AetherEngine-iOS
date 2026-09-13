/* AetherSaveFormat.c — Format helpers.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherSaveFormat.h"

u32 aether_save_checksum(const u8 *data, u32 size) {
    if (!data || size == 0) return 0;
    u32 h = 0x811C9DC5u;
    for (u32 i = 0; i < size; ++i) {
        h ^= (u32)data[i];
        h *= 16777619u;
        h ^= (h >> 13);
    }
    return h;
}
