/* 
 * File:   itv.h
 * Author: kenji123
 *
 * Created on April 7, 2015, 4:08 PM
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

