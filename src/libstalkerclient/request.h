/*
 *      Copyright (C) 2015  Jamal Edey
 *      http://www.kenshisoft.com/
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *  http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 */

#ifndef REQUEST_H
#define	REQUEST_H

#include "identity.h"
#include "param.h"

#ifdef	__cplusplus
extern "C" {
#endif


  struct sc_request_header;

  typedef struct sc_request_header {
    const char *name;
    char *value;
    struct sc_request_header *first;
    struct sc_request_header *prev;
    struct sc_request_header *next;
  } sc_request_header_t;

  typedef struct {
    const char *method;
    char query[1024];
    sc_request_header_t *headers;
  } sc_request_t;

  void sc_request_set_missing_required(sc_param_request_t *dst_params, sc_param_request_t *src_params);
  void sc_request_remove_default_non_required(sc_param_request_t *dst_params, sc_param_request_t *src_params);
  sc_request_header_t* sc_request_create_header(const char *name, char *value);
  sc_request_header_t* sc_request_link_header(sc_request_header_t *a, sc_request_header_t *b);
  void sc_request_build_headers(sc_identity_t *identity, sc_request_t *request, sc_action_t action);
  void sc_request_build_query(sc_param_request_t *params, sc_request_t *request);
  bool sc_request_build(sc_identity_t *identity, sc_param_request_t *params, sc_request_t *request);
  void sc_request_free_header(sc_request_header_t *header);
  void sc_request_free_headers(sc_request_header_t *header);
  void sc_request_free(sc_request_t *request);


#ifdef	__cplusplus
}
#endif

#endif	/* REQUEST_H */

