/* 
 * File:   action.h
 * Author: kenji123
 *
 * Created on April 9, 2015, 11:19 PM
 */

#ifndef ACTION_H
#define	ACTION_H

#ifdef	__cplusplus
extern "C" {
#endif

  typedef enum {
    // STB
    STB_HANDSHAKE,
    STB_GET_PROFILE,

    // ITV
    ITV_GET_ALL_CHANNELS,
    ITV_GET_ORDERED_LIST,
    ITV_CREATE_LINK,
    ITV_GET_GENRES,
    ITV_GET_EPG_INFO
  } sc_action_t;


#ifdef	__cplusplus
}
#endif

#endif	/* ACTION_H */

