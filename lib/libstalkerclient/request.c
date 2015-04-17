#include "request.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "stb.h"
#include "itv.h"
#include "util.h"

void sc_request_set_missing_required(sc_param_request_t *dst_params, sc_param_request_t *src_params) {
  sc_param_t *src_param;
  sc_param_t *dst_param;

  src_param = src_params->param;
  while (src_param) {
    if (!(dst_param = sc_param_get(dst_params, src_param->name))
      && src_param->required) {
      sc_param_t *copy = NULL;
      if ((copy = sc_param_copy(src_param))) {
        fprintf(stderr, "appending %s\n", src_param->name);
        sc_param_append(dst_params, copy);
      }
    }

    src_param = src_param->next;
  }
}

void sc_request_remove_default_non_required(sc_param_request_t *dst_params, sc_param_request_t *src_params) {
  sc_param_t *src_param;
  sc_param_t *dst_param;

  src_param = src_params->param;
  while (src_param) {
    bool destroy = true;

    if ((dst_param = sc_param_get(dst_params, src_param->name))) {
      switch (src_param->type) {
        case SC_STRING:
          if (strcmp(dst_param->value.string, src_param->value.string) != 0) {
            free(dst_param->value.string);
            dst_param->value.string = src_param->value.string;
            destroy = false;
          }
          break;
        case SC_INTEGER:
          if (dst_param->value.integer != src_param->value.integer) {
            dst_param->value.integer = src_param->value.integer;
            destroy = false;
          }
          break;
        case SC_BOOLEAN:
          if (dst_param->value.boolean != src_param->value.boolean) {
            dst_param->value.boolean = src_param->value.boolean;
            destroy = false;
          }
          break;
      }

      if (!dst_param->required && destroy) {
        fprintf(stderr, "destroying %s\n", dst_param->name);
        sc_param_destroy(dst_params, dst_param);
      }
    }

    src_param = src_param->next;
  }
}

sc_request_header_t* sc_request_create_header(const char *name, char *value) {
  sc_request_header_t *header;

  header = (sc_request_header_t *) malloc(sizeof (sc_request_header_t));
  header->name = name;
  header->value = sc_util_strcpy(value);

  header->first = NULL;
  header->prev = NULL;
  header->next = NULL;

  return header;
}

sc_request_header_t* sc_request_link_header(sc_request_header_t *a, sc_request_header_t *b) {
  b->first = a->first;
  b->prev = a;
  a->next = b;

  return b;
}

void sc_request_build_headers(sc_identity_t *identity, sc_request_t *request, sc_action_t action) {
  sc_request_header_t *header;
  char buffer[256];

  memset(&buffer, 0, sizeof (buffer));
  sprintf(buffer, "mac=%s; stb_lang=%s; timezone=%s",
    identity->mac, identity->lang, identity->time_zone);
  header = sc_request_create_header("Cookie", buffer);

  if (!request->headers) {
    header->first = header;
    request->headers = header;
  } else {
    header = sc_request_link_header(request->headers, header);
  }

  if (action != STB_HANDSHAKE) {
    memset(&buffer, 0, sizeof (buffer));
    sprintf(buffer, "Bearer %s", identity->auth_token);
    header = sc_request_link_header(header, sc_request_create_header("Authorization", buffer));
  }

  header->next = NULL;
}

void sc_request_build_query(sc_param_request_t *params, sc_request_t *request) {
  sc_param_t *param;
  char buffer[1024];
  char str[1024];
  size_t pos = strlen(request->query);
  size_t len = 0;

  param = params->param;
  while (param) {
    memset(&buffer, 0, sizeof (buffer));
    memset(&str, 0, sizeof (str));

    switch (param->type) {
      case SC_STRING:
        sprintf(buffer, "%s", param->value.string);
        break;
      case SC_INTEGER:
        sprintf(buffer, "%d", param->value.integer);
        break;
      case SC_BOOLEAN:
        sprintf(buffer, "%d", param->value.boolean ? 1 : 0);
        break;
    }

    sprintf(str, "%s=%s&", param->name, buffer);

    len = strlen(str);
    strncpy(&request->query[pos], str, len);

    /*fprintf(stderr, "param: %s | len: %d | pos: %d\n",
            param->name, len, pos);*/

    pos += len;
    param = param->next;
  }

  // remove trailing '&'
  request->query[pos - 1] = '\0';
}

bool sc_request_build(sc_identity_t *identity, sc_param_request_t *params, sc_request_t *request) {
  sc_param_request_t *final_params;

  final_params = (sc_param_request_t *) malloc(sizeof (sc_param_request_t));
  memset(final_params, 0, sizeof (final_params));
  final_params->action = params->action;

  switch (final_params->action) {
    case STB_HANDSHAKE:
    case STB_GET_PROFILE:
      sc_stb_defaults(final_params);
      sc_stb_prep_request(params, request);
      break;
    case ITV_GET_ALL_CHANNELS:
    case ITV_GET_ORDERED_LIST:
    case ITV_CREATE_LINK:
    case ITV_GET_GENRES:
    case ITV_GET_EPG_INFO:
      sc_itv_defaults(final_params);
      sc_itv_prep_request(params, request);
      break;
  }

  sc_request_set_missing_required(params, final_params);
  sc_request_remove_default_non_required(final_params, params);

  sc_request_build_headers(identity, request, final_params->action);
  sc_request_build_query(final_params, request);

  return true;
}

void sc_request_free_header(sc_request_header_t *header) {
  free(header->value);
  free(header);
  header = NULL;
}

void sc_request_free_headers(sc_request_header_t *header) {
  while (header) {
    sc_request_header_t *next;
    next = header->next;

    sc_request_free_header(header);

    header = next;
  }
}

void sc_request_free(sc_request_t *request) {
  sc_request_free_headers(request->headers);
  free(request);
  request = NULL;
}
