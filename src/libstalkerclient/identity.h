/* 
 * File:   identity.h
 * Author: kenji123
 *
 * Created on April 12, 2015, 3:29 PM
 */

#ifndef IDENTITY_H
#define	IDENTITY_H

#ifdef	__cplusplus
extern "C" {
#endif

  typedef struct {
    const char *mac;
    const char *lang;
    const char *time_zone;
    const char *auth_token;
  } sc_identity_t;


#ifdef	__cplusplus
}
#endif

#endif	/* IDENTITY_H */

