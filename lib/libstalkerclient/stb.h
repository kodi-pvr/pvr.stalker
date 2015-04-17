/* 
 * File:   stb.h
 * Author: kenji123
 *
 * Created on April 7, 2015, 3:50 PM
 */

#ifndef STB_H
#define	STB_H

#include <stdbool.h>

//#include "type.h"
#include "param.h"
#include "request.h"

#ifdef	__cplusplus
extern "C" {
#endif


  bool sc_stb_handshake_defaults(sc_param_request_t *params);
  bool sc_stb_get_profile_defaults(sc_param_request_t *params);
  bool sc_stb_defaults(sc_param_request_t *params);
  bool sc_stb_prep_request(sc_param_request_t *params, sc_request_t *request);


#ifdef	__cplusplus
}
#endif

#endif	/* STB_H */

