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

#ifndef ITV_H
#define	ITV_H

#include <stdbool.h>

#include "param.h"
#include "request.h"

#ifdef	__cplusplus
extern "C" {
#endif


  bool sc_itv_get_all_channels_defaults(sc_param_request_t *params);
  bool sc_itv_get_ordered_list_defaults(sc_param_request_t *params);
  bool sc_itv_create_link_defaults(sc_param_request_t *params);
  bool sc_itv_get_genres_defaults(sc_param_request_t *params);
  bool sc_itv_get_epg_info_defaults(sc_param_request_t *params);
  bool sc_itv_defaults(sc_param_request_t *params);
  bool sc_itv_prep_request(sc_param_request_t *params, sc_request_t *request);


#ifdef	__cplusplus
}
#endif

#endif	/* ITV_H */

