/**
 * libkeccak – Keccak-family hashing library
 * 
 * Copyright © 2014  Mattias Andrée (maandree@member.fsf.org)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBKECCAK_STATE_H
#define LIBKECCAK_STATE_H  1


#include "spec.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>



/**
 * Satastructure that describes the state of a hashing process
 * 
 * The `char`-size of the output hashsum is calculated by `(.n + 7) >> 3`
 */
typedef struct libkeccak_state
{
  /**
   * The bitrate
   */
  long r;
  
  /**
   * The capacity
   */
  long c;
  
  /**
   * The output size
   */
  long n;
  
  /**
   * The state size
   */
  long b;
  
  /**
   * The word size
   */
  long w;
  
  /**
   * The word mask
   */
  int_fast64_t wmod;
  
  /**
   * ℓ, the binary logarithm of the word size
   */
  long l;
  
  /**
   * 12 + 2ℓ, the number of rounds
   */
  long nr;
  
  /**
   * The lanes (state)
   */
  int_fast64_t S[25];
  
  /**
   * Pointer for `M`
   */
  size_t mptr;
  
  /**
   * Size of `M`
   */
  size_t mlen;
  
  /**
   * Left over water to fill the sponge with at next update
   */
  char* M;
  
} libkeccak_state_t;



/**
 * Initialise a state according to hashing specifications
 * 
 * @param   state  The state that should be initialised
 * @param   spec   The specifications for the state
 * @return         Zero on success, -1 on error
 */
__attribute__((leaf, nonnull))
int libkeccak_state_initialise(libkeccak_state_t* restrict state, const libkeccak_spec_t* restrict spec);


/**
 * Release resources allocation for a state without wiping sensitive data
 * 
 * @param  state  The state that should be destroyed
 */
static inline __attribute__((leaf))
void libkeccak_state_fast_destroy(libkeccak_state_t* restrict state)
{
  if (state == NULL)
    return;
  free(state->M);
  state->M = NULL;
}


/**
 * Wipe sensitive data wihout freeing any data, this is intended
 * be called from `libkeccak_state_destroy`
 * 
 * @param  state  The state that should be wipe
 */
__attribute__((leaf, nonnull, nothrow, optimize("-O0")))
void libkeccak_state_wipe(volatile libkeccak_state_t* restrict state);


/**
 * Release resources allocation for a state and wipe sensitive data
 * 
 * @param  state  The state that should be destroyed
 */
static inline __attribute__((unused, optimize("-O0")))
void libkeccak_state_destroy(volatile libkeccak_state_t* restrict state)
{
  if (state == NULL)
    return;
  libkeccak_state_wipe(state);
  free(state->M);
  state->M = NULL;
}


/**
 * Wrapper for `libkeccak_state_initialise` that also allocates the states
 * 
 * @param   spec   The specifications for the state
 * @return         The state, `NULL` on error
 */
static inline __attribute__((nonnull, unused, warn_unused_result))
libkeccak_state_t* libkeccak_state_create(const libkeccak_spec_t* restrict spec)
{
  libkeccak_state_t* restrict state = malloc(sizeof(libkeccak_state_t));
  int saved_errno;
  if ((state == NULL) || libkeccak_state_initialise(state))
    return saved_errno = errno, free(state), errno = saved_errno, NULL;
  return state;
}


/**
 * Wrapper for `libkeccak_state_fast_destroy` that also frees the allocation of the state
 * 
 * @param  state  The state that should be freed
 */
static inline __attribute__((unused))
void libkeccak_state_fast_free(libkeccak_state_t* restrict state)
{
  libkeccak_state_fast_destroy(state);
  free(state);
}


/**
 * Wrapper for `libkeccak_state_destroy` that also frees the allocation of the state
 * 
 * @param  state  The state that should be freed
 */
static inline __attribute__((unused, optimize("-O0")))
void libkeccak_state_free(volatile libkeccak_state_t* restrict state)
{
  libkeccak_state_destroy(state);
  free(state);
}


/**
 * Make a copy of a state
 * 
 * @param   dest  The slot for the duplicate, must not be initialised (memory leak otherwise)
 * @param   src   The state to duplicate
 * @return        Zero on success, -1 on error
 */
__attribute__((leaf, nonnull))
int libkeccak_state_copy(libkeccak_state_t* restrict dest, const libkeccak_state_t* restrict src);


/**
 * A wrapper for `libkeccak_state_copy` that also allocates the duplicate
 * 
 * @param   src   The state to duplicate
 * @return        The duplicate, `NULL` on error
 */
static inline __attribute__((nonnull, unused, warn_unused_result))
libkeccak_state_t* libkeccak_state_duplicate(const libkeccak_state_t* restrict src)
{
  libkeccak_state_t* restrict dest = malloc(sizeof(libkeccak_state_t));
  int saved_errno;
  if ((dest == NULL) || libkeccak_state_copy(dest, src))
    return saved_errno = errno, libkeccak_state_free(dest), errno = saved_errno, NULL;
  return dest;
}


/**
 * Calculates the allocation size required for the second argument
 * of `libkeccak_state_marshal` (`char* restrict data)`)
 * 
 * @param   state  The state as it will be marshalled by a subsequent call to `libkeccak_state_marshal`
 * @return         The allocation size needed for the buffer to which the state will be marshalled
 */
static inline __attribute__((leaf, nonnull, nothrow, unused, warn_unused_result, pure))
size_t libkeccak_state_marshal_size(const libkeccak_state_t* restrict state)
{
  return sizeof(libkeccak_state_t) - sizeof(char*) + state->mptr * sizeof(char);
}


/**
 * Marshal a `libkeccak_state_t` into a buffer
 * 
 * @param   state  The state to marshal
 * @param   data   The output buffer
 * @return         The number of bytes stored to `data`
 */
__attribute__((leaf, nonnull, nothrow))
size_t libkeccak_state_marshal(const libkeccak_state_t* restrict state, char* restrict data);


/**
 * Unmarshal a `libkeccak_state_t` from a buffer
 * 
 * @param   state  The slot for the unmarshalled state, must not be initialised (memory leak otherwise)
 * @param   data   The input buffer
 * @return         The number of bytes read from `data`, 0 on error
 */
__attribute__((leaf, nonnull))
size_t libkeccak_state_unmarshal(libkeccak_state_t* restrict state, const char* restrict data);


/**
 * Gets the number of bytes the `libkeccak_state_t` stored
 * at the beginning of `data` occupies
 * 
 * @param   data  The data buffer
 * @return        The byte size of the stored state
 */
__attribute__((leaf, nonnull, nothrow, warn_unused_result, pure))
size_t libkeccak_state_unmarshal_skip(const char* restrict data);


#undef
