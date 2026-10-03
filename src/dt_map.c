/*
 * dt_map.c: Associative arrays for Unit 5, Section E.
 *
 * An array does not store its indices. This map stores its keys.
 * An array calculates a position with one subtraction.
 * The map calculates a hash and then compares keys in one bucket.
 *
 * Hashing turns the key into a bucket number. Compare all keys in that bucket
 * because two keys can select it. Use a linked list for each bucket. Start the
 * unsigned accumulator at 14695981039346656037ULL. For each unsigned byte,
 * exclusive-or the byte into it and multiply by 1099511628211ULL.
 *
 * A separate list stores insertion order for stable output. dt_map_key_at
 * reads this list. Updating a key preserves its position. Removing and
 * reinserting a key moves it to the end.
 */

#include "dt.h"

#include <stdlib.h>
#include <string.h>

// an entry in a bucket chain and insertion order list
struct dt_entry
{
    char *key;
    dt_value value;
    struct dt_entry *next;
};

struct dt_map
{
    struct dt_entry **buckets; // bucket array, each bucket is a linked list
    size_t bucket_count;

    struct dt_entry **order; // insertion order array, each element points to an entry
    size_t order_len;
    size_t order_cap;

    size_t count; // number of keys in the map
};

/*
 * dt_map_new builds an empty map. It returns NULL after an allocation failure.
 */
dt_map *dt_map_new(void)
{
    dt_map *m = malloc(sizeof *m);
    if (m == NULL)
    {
        return NULL;
    }

    m->bucket_count = 16; /* initial bucket count */
    m->buckets = calloc(m->bucket_count, sizeof(struct dt_entry *));
    if (m->buckets == NULL)
    {
        free(m);
        return NULL;
    }

    m->order = NULL;
    m->order_len = 0;
    m->order_cap = 0;

    m->count = 0;

    return m;
}

static unsigned long long hash_function(const char *key)
{
    unsigned long long hash = 14695981039346656037ULL;
    while (*key)
    {
        hash ^= (unsigned char)(*key);
        hash *= 1099511628211ULL;
        key++;
    }
    return hash;
}

static struct dt_entry *find_entry(const dt_map *m, const char *key)
{
    unsigned long long hash = hash_function(key);
    size_t bucket_index = hash % m->bucket_count;
    struct dt_entry *bucket = m->buckets[bucket_index];
    while (bucket != NULL)
    {
        if (strcmp(bucket->key, key) == 0)
        {
            return bucket;
        }
        bucket = bucket->next;
    }
    return NULL;
}
/*
 * dt_map_free releases each entry, copied key, order array, and map.
 * It accepts NULL. The environment owns the values.
 */
void dt_map_free(dt_map *m)
{
    if (m == NULL)
    {
        return;
    }
    else{
        for (size_t i = 0; i < m->order_len; i++)
        {
            struct dt_entry *entry = m->order[i];
            free(entry->key);
            free(entry);
        }
        free(m->order);
        free(m->buckets);
        free(m);
    }
}

/*
 * dt_map_len returns the number of keys in constant time.
 */
size_t dt_map_len(const dt_map *m)
{
    return m->count;   
}

/*
 * dt_map_put binds v to key.
 * An existing key keeps its insertion position. A new key becomes the last key.
 * Copy each new key because the caller owns the source buffer.
 * Return DT_ERR_CAPACITY after an allocation failure.
 */
dt_status dt_map_put(dt_map *m, const char *key, dt_value v)
{
    struct dt_entry *existing_entry = find_entry(m, key);
    if (existing_entry != NULL)
    {
        // if key alr exists in map, just set its value
        existing_entry->value = v;
        return DT_OK;
    }
    else
    {
        // allocate memory for the new key
        char *k = malloc(strlen(key) + 1);
        if (k == NULL)
        {
            return DT_ERR_CAPACITY;
        }
        // allocate memory for the new entry
        struct dt_entry *new_entry = malloc(sizeof(struct dt_entry));
        if (new_entry == NULL)
        {
            free(k);
            return DT_ERR_CAPACITY;
        }
        // if the malloc doesnt fail, copy the properties into the allocated dt_entry
        strcpy(k, key);
        new_entry->key = k;
        new_entry->value = v;

        struct dt_entry **new_order =
            realloc(m->order, (m->order_len + 1) * sizeof(struct dt_entry *));
        if (new_order == NULL)
        {
            free(k);
            free(new_entry);
            return DT_ERR_CAPACITY;
        }
        m->order = new_order;
        m->order[m->order_len] = new_entry;
        m->order_len++;

        // recalc bucket index
        unsigned long long hash = hash_function(key);
        size_t bucket_index = hash % m->bucket_count;
        new_entry->next = m->buckets[bucket_index];
        m->buckets[bucket_index] = new_entry;

        m->count++;
        return DT_OK;
    }
}

/*
 * dt_map_get writes the value for key to *out.
 * It returns DT_ERR_KEY and does not change *out when the key is absent.
 * An absent key differs from a nil value.
 */
dt_status dt_map_get(const dt_map *m, const char *key, dt_value *out)
{
    struct dt_entry *entry = find_entry(m, key);
    if (entry != NULL)
    {
        *out = entry->value;
        return DT_OK;
    }
    else
    {
        return DT_ERR_KEY;
    }
}

/*
 * dt_map_remove removes key from its bucket and insertion position.
 * It releases the copied key. It returns DT_ERR_KEY when the key is absent.
 */
dt_status dt_map_remove(dt_map *m, const char *key)
{
    // recalc bucket index
    unsigned long long hash = hash_function(key);
    size_t bucket_index = hash % m->bucket_count;

    // two pointers
    struct dt_entry *current = m->buckets[bucket_index];
    struct dt_entry *prev = NULL;

    // traverse together to find entry
    while (current != NULL)
    {
        if (strcmp(current->key, key) == 0)
        {
            // if found, remove from bucket linked list (doesnt delete the entry from memory)
            if (prev == NULL)
            {
                m->buckets[bucket_index] = current->next;
            }
            else
            {
                prev->next = current->next;
            }
            break;
        }
        prev = current;
        current = current->next;
    }
    if (current == NULL)
    {
        return DT_ERR_KEY; // key not found
    }

    // remove from order array
    for (size_t i = 0; i < m->order_len; i++)
    {
        if (m->order[i] == current)
        {
            for (size_t j = i; j < m->order_len - 1; j++)
            {
                m->order[j] = m->order[j + 1];
            }
            m->order_len--;
            break;
        }
    }

    free(current->key);
    free(current);
    m->count--;
    return DT_OK;
}

/*
 * dt_map_key_at writes the key at insertion position index to *out.
 * It returns DT_ERR_RANGE and does not change *out for an invalid index.
 */
dt_status dt_map_key_at(const dt_map *m, size_t index, const char **out)
{
    if (index >= m->order_len)
    {
        return DT_ERR_RANGE;
    }
    else
    {
        *out = m->order[index]->key;
        return DT_OK;
    }
}
