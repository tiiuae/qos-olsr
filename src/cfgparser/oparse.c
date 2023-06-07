/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "src/cfgparser/oparse.y"


/*
 * The olsr.org Optimized Link-State Routing daemon (olsrd)
 *
 * (c) by the OLSR project
 *
 * See our Git repository to find out who worked on this file
 * and thus is a copyright holder on it.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * * Neither the name of olsr.org, olsrd nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Visit http://www.olsr.org for more information.
 *
 * If you find this software useful feel free to make a donation
 * to the project. For more information see the website or contact
 * the copyright holders.
 *
 */

#include "olsrd_conf.h"
#include "defs.h"
#include "ipcalc.h"
#include "net_olsr.h"
#include "link_set.h"
#include "olsr.h"
#include "egressTypes.h"
#include "gateway.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>

#define PARSER_DEBUG 1

#if defined PARSER_DEBUG && PARSER_DEBUG
#define PARSER_DEBUG_PRINTF(x, args...)   printf(x, ##args)
#else
#define PARSER_DEBUG_PRINTF(x, args...)   do { } while (0)
#endif

#define SET_IFS_CONF(ifs, ifcnt, field, value) do { \
	for (; ifcnt>0; ifs=ifs->next, ifcnt--) { \
    ifs->cnfi->field = (value); \
    ifs->cnf->field = (value); \
	} \
} while (0)

#define YYSTYPE struct conf_token *

void yyerror(const char *);
int yylex(void);

static int ifs_in_curr_cfg = 0;

static int add_ipv6_addr(YYSTYPE ipaddr_arg, YYSTYPE prefixlen_arg);

static int lq_mult_helper(YYSTYPE ip_addr_arg, YYSTYPE mult_arg)
{
  union olsr_ip_addr addr;
  int i;
  struct olsr_if *walker;

#if defined PARSER_DEBUG && PARSER_DEBUG > 0
  printf("\tLinkQualityMult %s %0.2f\n",
         (ip_addr_arg != NULL) ? ip_addr_arg->string : "any",
         (double)mult_arg->floating);
#endif

  memset(&addr, 0, sizeof(addr));

  if (ip_addr_arg != NULL &&
     inet_pton(olsr_cnf->ip_version, ip_addr_arg->string, &addr) <= 0) {
    fprintf(stderr, "Cannot parse IP address %s.\n", ip_addr_arg->string);
    return -1;
  }

  walker = olsr_cnf->interfaces;

  for (i = 0; i < ifs_in_curr_cfg; i++) {
    struct olsr_lq_mult *mult = malloc(sizeof(*mult));
    if (mult == NULL) {
      fprintf(stderr, "Out of memory (LQ multiplier).\n");
      return -1;
    }

    mult->addr = addr;
    mult->value = (uint32_t)(mult_arg->floating * LINK_LOSS_MULTIPLIER);

    mult->next = walker->cnf->lq_mult;
    walker->cnfi->lq_mult = walker->cnf->lq_mult = mult;
    walker->cnf->orig_lq_mult_cnt++;
    walker->cnfi->orig_lq_mult_cnt=walker->cnf->orig_lq_mult_cnt;

    walker = walker->next;
  }

  if (ip_addr_arg != NULL) {
    free(ip_addr_arg->string);
    free(ip_addr_arg);
  }

  free(mult_arg);

  return 0;
}

static int add_ipv6_addr(YYSTYPE ipaddr_arg, YYSTYPE prefixlen_arg)
{
  union olsr_ip_addr ipaddr;
  PARSER_DEBUG_PRINTF("HNA IPv6 entry: %s/%d\n", ipaddr_arg->string, prefixlen_arg->integer);

  if (olsr_cnf->ip_version != AF_INET6) {
    fprintf(stderr, "IPv6 addresses can only be used if \"IpVersion\" == 6, skipping HNA6.\n");
    olsr_startup_sleep(3);
  }
	else {
	  if(inet_pton(AF_INET6, ipaddr_arg->string, &ipaddr) <= 0) {
      fprintf(stderr, "ihna6entry: Failed converting IP address %s\n", ipaddr_arg->string);
      return 1;
    }

		if (prefixlen_arg->integer > 128) {
			fprintf(stderr, "ihna6entry: Illegal IPv6 prefix length %d\n", prefixlen_arg->integer);
			return 1;
		}

		/* Queue */
		ip_prefix_list_add(&olsr_cnf->hna_entries, &ipaddr, prefixlen_arg->integer);
	}
  free(ipaddr_arg->string);
  free(ipaddr_arg);
  free(prefixlen_arg);

  return 0;
}


#line 243 "src/cfgparser/oparse.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "oparse.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_TOK_SLASH = 3,                  /* TOK_SLASH  */
  YYSYMBOL_TOK_OPEN = 4,                   /* TOK_OPEN  */
  YYSYMBOL_TOK_CLOSE = 5,                  /* TOK_CLOSE  */
  YYSYMBOL_TOK_STRING = 6,                 /* TOK_STRING  */
  YYSYMBOL_TOK_INTEGER = 7,                /* TOK_INTEGER  */
  YYSYMBOL_TOK_FLOAT = 8,                  /* TOK_FLOAT  */
  YYSYMBOL_TOK_BOOLEAN = 9,                /* TOK_BOOLEAN  */
  YYSYMBOL_TOK_IPV6TYPE = 10,              /* TOK_IPV6TYPE  */
  YYSYMBOL_TOK_DEBUGLEVEL = 11,            /* TOK_DEBUGLEVEL  */
  YYSYMBOL_TOK_IPVERSION = 12,             /* TOK_IPVERSION  */
  YYSYMBOL_TOK_HNA4 = 13,                  /* TOK_HNA4  */
  YYSYMBOL_TOK_HNA6 = 14,                  /* TOK_HNA6  */
  YYSYMBOL_TOK_PLUGIN = 15,                /* TOK_PLUGIN  */
  YYSYMBOL_TOK_INTERFACE_DEFAULTS = 16,    /* TOK_INTERFACE_DEFAULTS  */
  YYSYMBOL_TOK_INTERFACE = 17,             /* TOK_INTERFACE  */
  YYSYMBOL_TOK_NOINT = 18,                 /* TOK_NOINT  */
  YYSYMBOL_TOK_TOS = 19,                   /* TOK_TOS  */
  YYSYMBOL_TOK_OLSRPORT = 20,              /* TOK_OLSRPORT  */
  YYSYMBOL_TOK_RTPROTO = 21,               /* TOK_RTPROTO  */
  YYSYMBOL_TOK_RTTABLE = 22,               /* TOK_RTTABLE  */
  YYSYMBOL_TOK_RTTABLE_DEFAULT = 23,       /* TOK_RTTABLE_DEFAULT  */
  YYSYMBOL_TOK_RTTABLE_TUNNEL = 24,        /* TOK_RTTABLE_TUNNEL  */
  YYSYMBOL_TOK_RTTABLE_PRIORITY = 25,      /* TOK_RTTABLE_PRIORITY  */
  YYSYMBOL_TOK_RTTABLE_DEFAULTOLSR_PRIORITY = 26, /* TOK_RTTABLE_DEFAULTOLSR_PRIORITY  */
  YYSYMBOL_TOK_RTTABLE_TUNNEL_PRIORITY = 27, /* TOK_RTTABLE_TUNNEL_PRIORITY  */
  YYSYMBOL_TOK_RTTABLE_DEFAULT_PRIORITY = 28, /* TOK_RTTABLE_DEFAULT_PRIORITY  */
  YYSYMBOL_TOK_WILLINGNESS = 29,           /* TOK_WILLINGNESS  */
  YYSYMBOL_TOK_IPCCON = 30,                /* TOK_IPCCON  */
  YYSYMBOL_TOK_FIBMETRIC = 31,             /* TOK_FIBMETRIC  */
  YYSYMBOL_TOK_FIBMETRICDEFAULT = 32,      /* TOK_FIBMETRICDEFAULT  */
  YYSYMBOL_TOK_USEHYST = 33,               /* TOK_USEHYST  */
  YYSYMBOL_TOK_HYSTSCALE = 34,             /* TOK_HYSTSCALE  */
  YYSYMBOL_TOK_HYSTUPPER = 35,             /* TOK_HYSTUPPER  */
  YYSYMBOL_TOK_HYSTLOWER = 36,             /* TOK_HYSTLOWER  */
  YYSYMBOL_TOK_POLLRATE = 37,              /* TOK_POLLRATE  */
  YYSYMBOL_TOK_NICCHGSPOLLRT = 38,         /* TOK_NICCHGSPOLLRT  */
  YYSYMBOL_TOK_TCREDUNDANCY = 39,          /* TOK_TCREDUNDANCY  */
  YYSYMBOL_TOK_MPRCOVERAGE = 40,           /* TOK_MPRCOVERAGE  */
  YYSYMBOL_TOK_LQ_LEVEL = 41,              /* TOK_LQ_LEVEL  */
  YYSYMBOL_TOK_LQ_FISH = 42,               /* TOK_LQ_FISH  */
  YYSYMBOL_TOK_LQ_AGING = 43,              /* TOK_LQ_AGING  */
  YYSYMBOL_TOK_LQ_PLUGIN = 44,             /* TOK_LQ_PLUGIN  */
  YYSYMBOL_TOK_LQ_NAT_THRESH = 45,         /* TOK_LQ_NAT_THRESH  */
  YYSYMBOL_TOK_LQ_MULT = 46,               /* TOK_LQ_MULT  */
  YYSYMBOL_TOK_CLEAR_SCREEN = 47,          /* TOK_CLEAR_SCREEN  */
  YYSYMBOL_TOK_PLPARAM = 48,               /* TOK_PLPARAM  */
  YYSYMBOL_TOK_MIN_TC_VTIME = 49,          /* TOK_MIN_TC_VTIME  */
  YYSYMBOL_TOK_LOCK_FILE = 50,             /* TOK_LOCK_FILE  */
  YYSYMBOL_TOK_USE_NIIT = 51,              /* TOK_USE_NIIT  */
  YYSYMBOL_TOK_SMART_GW = 52,              /* TOK_SMART_GW  */
  YYSYMBOL_TOK_SMART_GW_ALWAYS_REMOVE_SERVER_TUNNEL = 53, /* TOK_SMART_GW_ALWAYS_REMOVE_SERVER_TUNNEL  */
  YYSYMBOL_TOK_SMART_GW_USE_COUNT = 54,    /* TOK_SMART_GW_USE_COUNT  */
  YYSYMBOL_TOK_SMART_GW_TAKEDOWN_PERCENTAGE = 55, /* TOK_SMART_GW_TAKEDOWN_PERCENTAGE  */
  YYSYMBOL_TOK_SMART_GW_INSTANCE_ID = 56,  /* TOK_SMART_GW_INSTANCE_ID  */
  YYSYMBOL_TOK_SMART_GW_POLICYROUTING_SCRIPT = 57, /* TOK_SMART_GW_POLICYROUTING_SCRIPT  */
  YYSYMBOL_TOK_SMART_GW_EGRESS_IFS = 58,   /* TOK_SMART_GW_EGRESS_IFS  */
  YYSYMBOL_TOK_SMART_GW_EGRESS_FILE = 59,  /* TOK_SMART_GW_EGRESS_FILE  */
  YYSYMBOL_TOK_SMART_GW_EGRESS_FILE_PERIOD = 60, /* TOK_SMART_GW_EGRESS_FILE_PERIOD  */
  YYSYMBOL_TOK_SMART_GW_STATUS_FILE = 61,  /* TOK_SMART_GW_STATUS_FILE  */
  YYSYMBOL_TOK_SMART_GW_OFFSET_TABLES = 62, /* TOK_SMART_GW_OFFSET_TABLES  */
  YYSYMBOL_TOK_SMART_GW_OFFSET_RULES = 63, /* TOK_SMART_GW_OFFSET_RULES  */
  YYSYMBOL_TOK_SMART_GW_ALLOW_NAT = 64,    /* TOK_SMART_GW_ALLOW_NAT  */
  YYSYMBOL_TOK_SMART_GW_PERIOD = 65,       /* TOK_SMART_GW_PERIOD  */
  YYSYMBOL_TOK_SMART_GW_STABLECOUNT = 66,  /* TOK_SMART_GW_STABLECOUNT  */
  YYSYMBOL_TOK_SMART_GW_THRESH = 67,       /* TOK_SMART_GW_THRESH  */
  YYSYMBOL_TOK_SMART_GW_WEIGHT_EXITLINK_UP = 68, /* TOK_SMART_GW_WEIGHT_EXITLINK_UP  */
  YYSYMBOL_TOK_SMART_GW_WEIGHT_EXITLINK_DOWN = 69, /* TOK_SMART_GW_WEIGHT_EXITLINK_DOWN  */
  YYSYMBOL_TOK_SMART_GW_WEIGHT_ETX = 70,   /* TOK_SMART_GW_WEIGHT_ETX  */
  YYSYMBOL_TOK_SMART_GW_DIVIDER_ETX = 71,  /* TOK_SMART_GW_DIVIDER_ETX  */
  YYSYMBOL_TOK_SMART_GW_MAX_COST_MAX_ETX = 72, /* TOK_SMART_GW_MAX_COST_MAX_ETX  */
  YYSYMBOL_TOK_SMART_GW_UPLINK = 73,       /* TOK_SMART_GW_UPLINK  */
  YYSYMBOL_TOK_SMART_GW_UPLINK_NAT = 74,   /* TOK_SMART_GW_UPLINK_NAT  */
  YYSYMBOL_TOK_SMART_GW_SPEED = 75,        /* TOK_SMART_GW_SPEED  */
  YYSYMBOL_TOK_SMART_GW_PREFIX = 76,       /* TOK_SMART_GW_PREFIX  */
  YYSYMBOL_TOK_SRC_IP_ROUTES = 77,         /* TOK_SRC_IP_ROUTES  */
  YYSYMBOL_TOK_MAIN_IP = 78,               /* TOK_MAIN_IP  */
  YYSYMBOL_TOK_SET_IPFORWARD = 79,         /* TOK_SET_IPFORWARD  */
  YYSYMBOL_TOK_HOSTLABEL = 80,             /* TOK_HOSTLABEL  */
  YYSYMBOL_TOK_NETLABEL = 81,              /* TOK_NETLABEL  */
  YYSYMBOL_TOK_MAXIPC = 82,                /* TOK_MAXIPC  */
  YYSYMBOL_TOK_IFMODE = 83,                /* TOK_IFMODE  */
  YYSYMBOL_TOK_IPV4MULTICAST = 84,         /* TOK_IPV4MULTICAST  */
  YYSYMBOL_TOK_IP4BROADCAST = 85,          /* TOK_IP4BROADCAST  */
  YYSYMBOL_TOK_IPV4BROADCAST = 86,         /* TOK_IPV4BROADCAST  */
  YYSYMBOL_TOK_IPV6MULTICAST = 87,         /* TOK_IPV6MULTICAST  */
  YYSYMBOL_TOK_IPV4SRC = 88,               /* TOK_IPV4SRC  */
  YYSYMBOL_TOK_IPV6SRC = 89,               /* TOK_IPV6SRC  */
  YYSYMBOL_TOK_IFWEIGHT = 90,              /* TOK_IFWEIGHT  */
  YYSYMBOL_TOK_HELLOINT = 91,              /* TOK_HELLOINT  */
  YYSYMBOL_TOK_HELLOVAL = 92,              /* TOK_HELLOVAL  */
  YYSYMBOL_TOK_TCINT = 93,                 /* TOK_TCINT  */
  YYSYMBOL_TOK_TCVAL = 94,                 /* TOK_TCVAL  */
  YYSYMBOL_TOK_MIDINT = 95,                /* TOK_MIDINT  */
  YYSYMBOL_TOK_MIDVAL = 96,                /* TOK_MIDVAL  */
  YYSYMBOL_TOK_HNAINT = 97,                /* TOK_HNAINT  */
  YYSYMBOL_TOK_HNAVAL = 98,                /* TOK_HNAVAL  */
  YYSYMBOL_TOK_AUTODETCHG = 99,            /* TOK_AUTODETCHG  */
  YYSYMBOL_TOK_IPV4_ADDR = 100,            /* TOK_IPV4_ADDR  */
  YYSYMBOL_TOK_IPV6_ADDR = 101,            /* TOK_IPV6_ADDR  */
  YYSYMBOL_TOK_DEFAULT = 102,              /* TOK_DEFAULT  */
  YYSYMBOL_TOK_AUTO = 103,                 /* TOK_AUTO  */
  YYSYMBOL_TOK_NONE = 104,                 /* TOK_NONE  */
  YYSYMBOL_TOK_COMMENT = 105,              /* TOK_COMMENT  */
  YYSYMBOL_YYACCEPT = 106,                 /* $accept  */
  YYSYMBOL_conf = 107,                     /* conf  */
  YYSYMBOL_stmt = 108,                     /* stmt  */
  YYSYMBOL_block = 109,                    /* block  */
  YYSYMBOL_hna4body = 110,                 /* hna4body  */
  YYSYMBOL_hna4stmts = 111,                /* hna4stmts  */
  YYSYMBOL_hna4stmt = 112,                 /* hna4stmt  */
  YYSYMBOL_hna6body = 113,                 /* hna6body  */
  YYSYMBOL_hna6stmts = 114,                /* hna6stmts  */
  YYSYMBOL_hna6stmt = 115,                 /* hna6stmt  */
  YYSYMBOL_ipcbody = 116,                  /* ipcbody  */
  YYSYMBOL_ipcstmts = 117,                 /* ipcstmts  */
  YYSYMBOL_ipcstmt = 118,                  /* ipcstmt  */
  YYSYMBOL_ifblock = 119,                  /* ifblock  */
  YYSYMBOL_ifnicks = 120,                  /* ifnicks  */
  YYSYMBOL_ifbody = 121,                   /* ifbody  */
  YYSYMBOL_ifdbody = 122,                  /* ifdbody  */
  YYSYMBOL_ifstmts = 123,                  /* ifstmts  */
  YYSYMBOL_ifstmt = 124,                   /* ifstmt  */
  YYSYMBOL_plbody = 125,                   /* plbody  */
  YYSYMBOL_plstmts = 126,                  /* plstmts  */
  YYSYMBOL_plstmt = 127,                   /* plstmt  */
  YYSYMBOL_ifdblock = 128,                 /* ifdblock  */
  YYSYMBOL_imaxipc = 129,                  /* imaxipc  */
  YYSYMBOL_ipchost = 130,                  /* ipchost  */
  YYSYMBOL_ipcnet = 131,                   /* ipcnet  */
  YYSYMBOL_iifweight = 132,                /* iifweight  */
  YYSYMBOL_isetifmode = 133,               /* isetifmode  */
  YYSYMBOL_isetipv4mc = 134,               /* isetipv4mc  */
  YYSYMBOL_isetipv6mc = 135,               /* isetipv6mc  */
  YYSYMBOL_isetipv4src = 136,              /* isetipv4src  */
  YYSYMBOL_isetipv6src = 137,              /* isetipv6src  */
  YYSYMBOL_isethelloint = 138,             /* isethelloint  */
  YYSYMBOL_isethelloval = 139,             /* isethelloval  */
  YYSYMBOL_isettcint = 140,                /* isettcint  */
  YYSYMBOL_isettcval = 141,                /* isettcval  */
  YYSYMBOL_isetmidint = 142,               /* isetmidint  */
  YYSYMBOL_isetmidval = 143,               /* isetmidval  */
  YYSYMBOL_isethnaint = 144,               /* isethnaint  */
  YYSYMBOL_isethnaval = 145,               /* isethnaval  */
  YYSYMBOL_isetautodetchg = 146,           /* isetautodetchg  */
  YYSYMBOL_isetlqmult = 147,               /* isetlqmult  */
  YYSYMBOL_idebug = 148,                   /* idebug  */
  YYSYMBOL_iipversion = 149,               /* iipversion  */
  YYSYMBOL_fibmetric = 150,                /* fibmetric  */
  YYSYMBOL_afibmetricdefault = 151,        /* afibmetricdefault  */
  YYSYMBOL_ihna4entry = 152,               /* ihna4entry  */
  YYSYMBOL_ihna6entry = 153,               /* ihna6entry  */
  YYSYMBOL_ifstart = 154,                  /* ifstart  */
  YYSYMBOL_ifnick = 155,                   /* ifnick  */
  YYSYMBOL_bnoint = 156,                   /* bnoint  */
  YYSYMBOL_atos = 157,                     /* atos  */
  YYSYMBOL_aolsrport = 158,                /* aolsrport  */
  YYSYMBOL_irtproto = 159,                 /* irtproto  */
  YYSYMBOL_irttable = 160,                 /* irttable  */
  YYSYMBOL_irttable_default = 161,         /* irttable_default  */
  YYSYMBOL_irttable_tunnel = 162,          /* irttable_tunnel  */
  YYSYMBOL_irttable_priority = 163,        /* irttable_priority  */
  YYSYMBOL_irttable_default_priority = 164, /* irttable_default_priority  */
  YYSYMBOL_irttable_tunnel_priority = 165, /* irttable_tunnel_priority  */
  YYSYMBOL_irttable_defaultolsr_priority = 166, /* irttable_defaultolsr_priority  */
  YYSYMBOL_awillingness = 167,             /* awillingness  */
  YYSYMBOL_busehyst = 168,                 /* busehyst  */
  YYSYMBOL_fhystscale = 169,               /* fhystscale  */
  YYSYMBOL_fhystupper = 170,               /* fhystupper  */
  YYSYMBOL_fhystlower = 171,               /* fhystlower  */
  YYSYMBOL_fpollrate = 172,                /* fpollrate  */
  YYSYMBOL_fnicchgspollrt = 173,           /* fnicchgspollrt  */
  YYSYMBOL_atcredundancy = 174,            /* atcredundancy  */
  YYSYMBOL_amprcoverage = 175,             /* amprcoverage  */
  YYSYMBOL_alq_level = 176,                /* alq_level  */
  YYSYMBOL_alq_fish = 177,                 /* alq_fish  */
  YYSYMBOL_alq_aging = 178,                /* alq_aging  */
  YYSYMBOL_amin_tc_vtime = 179,            /* amin_tc_vtime  */
  YYSYMBOL_alock_file = 180,               /* alock_file  */
  YYSYMBOL_alq_plugin = 181,               /* alq_plugin  */
  YYSYMBOL_anat_thresh = 182,              /* anat_thresh  */
  YYSYMBOL_bclear_screen = 183,            /* bclear_screen  */
  YYSYMBOL_suse_niit = 184,                /* suse_niit  */
  YYSYMBOL_bsmart_gw = 185,                /* bsmart_gw  */
  YYSYMBOL_bsmart_gw_always_remove_server_tunnel = 186, /* bsmart_gw_always_remove_server_tunnel  */
  YYSYMBOL_ismart_gw_use_count = 187,      /* ismart_gw_use_count  */
  YYSYMBOL_ismart_gw_takedown_percentage = 188, /* ismart_gw_takedown_percentage  */
  YYSYMBOL_ssmart_gw_instance_id = 189,    /* ssmart_gw_instance_id  */
  YYSYMBOL_ssmart_gw_policyrouting_script = 190, /* ssmart_gw_policyrouting_script  */
  YYSYMBOL_ssgw_egress_ifs = 191,          /* ssgw_egress_ifs  */
  YYSYMBOL_sgw_egress_ifs = 192,           /* sgw_egress_ifs  */
  YYSYMBOL_sgw_egress_if = 193,            /* sgw_egress_if  */
  YYSYMBOL_ssmart_gw_egress_file = 194,    /* ssmart_gw_egress_file  */
  YYSYMBOL_ismart_gw_egress_file_period = 195, /* ismart_gw_egress_file_period  */
  YYSYMBOL_ssmart_gw_status_file = 196,    /* ssmart_gw_status_file  */
  YYSYMBOL_ismart_gw_offset_tables = 197,  /* ismart_gw_offset_tables  */
  YYSYMBOL_ismart_gw_offset_rules = 198,   /* ismart_gw_offset_rules  */
  YYSYMBOL_bsmart_gw_allow_nat = 199,      /* bsmart_gw_allow_nat  */
  YYSYMBOL_ismart_gw_period = 200,         /* ismart_gw_period  */
  YYSYMBOL_asmart_gw_stablecount = 201,    /* asmart_gw_stablecount  */
  YYSYMBOL_asmart_gw_thresh = 202,         /* asmart_gw_thresh  */
  YYSYMBOL_asmart_gw_weight_exitlink_up = 203, /* asmart_gw_weight_exitlink_up  */
  YYSYMBOL_asmart_gw_weight_exitlink_down = 204, /* asmart_gw_weight_exitlink_down  */
  YYSYMBOL_asmart_gw_weight_etx = 205,     /* asmart_gw_weight_etx  */
  YYSYMBOL_asmart_gw_divider_etx = 206,    /* asmart_gw_divider_etx  */
  YYSYMBOL_ssmart_gw_uplink = 207,         /* ssmart_gw_uplink  */
  YYSYMBOL_ismart_gw_speed = 208,          /* ismart_gw_speed  */
  YYSYMBOL_bsmart_gw_uplink_nat = 209,     /* bsmart_gw_uplink_nat  */
  YYSYMBOL_ismart_gw_prefix = 210,         /* ismart_gw_prefix  */
  YYSYMBOL_bsrc_ip_routes = 211,           /* bsrc_ip_routes  */
  YYSYMBOL_amain_ip = 212,                 /* amain_ip  */
  YYSYMBOL_bset_ipforward = 213,           /* bset_ipforward  */
  YYSYMBOL_plblock = 214,                  /* plblock  */
  YYSYMBOL_plparam = 215,                  /* plparam  */
  YYSYMBOL_vcomment = 216                  /* vcomment  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   253

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  106
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  111
/* YYNRULES -- Number of rules.  */
#define YYNRULES  229
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  340

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   360


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   286,   286,   287,   288,   291,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,   304,   305,
     306,   307,   308,   309,   310,   311,   312,   313,   314,   315,
     316,   317,   318,   319,   320,   321,   322,   323,   324,   325,
     326,   327,   328,   329,   330,   331,   332,   333,   334,   335,
     336,   337,   338,   339,   340,   341,   342,   343,   344,   345,
     346,   347,   348,   349,   350,   351,   354,   355,   356,   357,
     358,   359,   362,   365,   365,   368,   369,   372,   375,   375,
     378,   379,   382,   385,   385,   388,   389,   390,   391,   394,
     397,   397,   400,   403,   417,   417,   420,   421,   422,   423,
     424,   425,   426,   427,   428,   429,   430,   431,   432,   433,
     434,   435,   436,   437,   438,   441,   444,   444,   447,   448,
     451,   489,   496,   513,   536,   560,   581,   597,   617,   637,
     657,   680,   701,   713,   725,   737,   749,   762,   774,   786,
     798,   811,   818,   825,   833,   842,   862,   882,   890,   924,
     960,   966,   974,   981,  1040,  1048,  1057,  1065,  1073,  1079,
    1087,  1093,  1101,  1107,  1115,  1121,  1127,  1135,  1141,  1147,
    1155,  1161,  1167,  1175,  1181,  1187,  1195,  1204,  1212,  1220,
    1228,  1236,  1244,  1252,  1260,  1268,  1276,  1284,  1292,  1300,
    1308,  1317,  1325,  1333,  1341,  1349,  1357,  1365,  1373,  1382,
    1391,  1394,  1394,  1397,  1472,  1481,  1489,  1498,  1506,  1514,
    1522,  1530,  1538,  1546,  1554,  1562,  1570,  1578,  1586,  1609,
    1619,  1627,  1639,  1653,  1665,  1677,  1690,  1699,  1745,  1774
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "TOK_SLASH",
  "TOK_OPEN", "TOK_CLOSE", "TOK_STRING", "TOK_INTEGER", "TOK_FLOAT",
  "TOK_BOOLEAN", "TOK_IPV6TYPE", "TOK_DEBUGLEVEL", "TOK_IPVERSION",
  "TOK_HNA4", "TOK_HNA6", "TOK_PLUGIN", "TOK_INTERFACE_DEFAULTS",
  "TOK_INTERFACE", "TOK_NOINT", "TOK_TOS", "TOK_OLSRPORT", "TOK_RTPROTO",
  "TOK_RTTABLE", "TOK_RTTABLE_DEFAULT", "TOK_RTTABLE_TUNNEL",
  "TOK_RTTABLE_PRIORITY", "TOK_RTTABLE_DEFAULTOLSR_PRIORITY",
  "TOK_RTTABLE_TUNNEL_PRIORITY", "TOK_RTTABLE_DEFAULT_PRIORITY",
  "TOK_WILLINGNESS", "TOK_IPCCON", "TOK_FIBMETRIC", "TOK_FIBMETRICDEFAULT",
  "TOK_USEHYST", "TOK_HYSTSCALE", "TOK_HYSTUPPER", "TOK_HYSTLOWER",
  "TOK_POLLRATE", "TOK_NICCHGSPOLLRT", "TOK_TCREDUNDANCY",
  "TOK_MPRCOVERAGE", "TOK_LQ_LEVEL", "TOK_LQ_FISH", "TOK_LQ_AGING",
  "TOK_LQ_PLUGIN", "TOK_LQ_NAT_THRESH", "TOK_LQ_MULT", "TOK_CLEAR_SCREEN",
  "TOK_PLPARAM", "TOK_MIN_TC_VTIME", "TOK_LOCK_FILE", "TOK_USE_NIIT",
  "TOK_SMART_GW", "TOK_SMART_GW_ALWAYS_REMOVE_SERVER_TUNNEL",
  "TOK_SMART_GW_USE_COUNT", "TOK_SMART_GW_TAKEDOWN_PERCENTAGE",
  "TOK_SMART_GW_INSTANCE_ID", "TOK_SMART_GW_POLICYROUTING_SCRIPT",
  "TOK_SMART_GW_EGRESS_IFS", "TOK_SMART_GW_EGRESS_FILE",
  "TOK_SMART_GW_EGRESS_FILE_PERIOD", "TOK_SMART_GW_STATUS_FILE",
  "TOK_SMART_GW_OFFSET_TABLES", "TOK_SMART_GW_OFFSET_RULES",
  "TOK_SMART_GW_ALLOW_NAT", "TOK_SMART_GW_PERIOD",
  "TOK_SMART_GW_STABLECOUNT", "TOK_SMART_GW_THRESH",
  "TOK_SMART_GW_WEIGHT_EXITLINK_UP", "TOK_SMART_GW_WEIGHT_EXITLINK_DOWN",
  "TOK_SMART_GW_WEIGHT_ETX", "TOK_SMART_GW_DIVIDER_ETX",
  "TOK_SMART_GW_MAX_COST_MAX_ETX", "TOK_SMART_GW_UPLINK",
  "TOK_SMART_GW_UPLINK_NAT", "TOK_SMART_GW_SPEED", "TOK_SMART_GW_PREFIX",
  "TOK_SRC_IP_ROUTES", "TOK_MAIN_IP", "TOK_SET_IPFORWARD", "TOK_HOSTLABEL",
  "TOK_NETLABEL", "TOK_MAXIPC", "TOK_IFMODE", "TOK_IPV4MULTICAST",
  "TOK_IP4BROADCAST", "TOK_IPV4BROADCAST", "TOK_IPV6MULTICAST",
  "TOK_IPV4SRC", "TOK_IPV6SRC", "TOK_IFWEIGHT", "TOK_HELLOINT",
  "TOK_HELLOVAL", "TOK_TCINT", "TOK_TCVAL", "TOK_MIDINT", "TOK_MIDVAL",
  "TOK_HNAINT", "TOK_HNAVAL", "TOK_AUTODETCHG", "TOK_IPV4_ADDR",
  "TOK_IPV6_ADDR", "TOK_DEFAULT", "TOK_AUTO", "TOK_NONE", "TOK_COMMENT",
  "$accept", "conf", "stmt", "block", "hna4body", "hna4stmts", "hna4stmt",
  "hna6body", "hna6stmts", "hna6stmt", "ipcbody", "ipcstmts", "ipcstmt",
  "ifblock", "ifnicks", "ifbody", "ifdbody", "ifstmts", "ifstmt", "plbody",
  "plstmts", "plstmt", "ifdblock", "imaxipc", "ipchost", "ipcnet",
  "iifweight", "isetifmode", "isetipv4mc", "isetipv6mc", "isetipv4src",
  "isetipv6src", "isethelloint", "isethelloval", "isettcint", "isettcval",
  "isetmidint", "isetmidval", "isethnaint", "isethnaval", "isetautodetchg",
  "isetlqmult", "idebug", "iipversion", "fibmetric", "afibmetricdefault",
  "ihna4entry", "ihna6entry", "ifstart", "ifnick", "bnoint", "atos",
  "aolsrport", "irtproto", "irttable", "irttable_default",
  "irttable_tunnel", "irttable_priority", "irttable_default_priority",
  "irttable_tunnel_priority", "irttable_defaultolsr_priority",
  "awillingness", "busehyst", "fhystscale", "fhystupper", "fhystlower",
  "fpollrate", "fnicchgspollrt", "atcredundancy", "amprcoverage",
  "alq_level", "alq_fish", "alq_aging", "amin_tc_vtime", "alock_file",
  "alq_plugin", "anat_thresh", "bclear_screen", "suse_niit", "bsmart_gw",
  "bsmart_gw_always_remove_server_tunnel", "ismart_gw_use_count",
  "ismart_gw_takedown_percentage", "ssmart_gw_instance_id",
  "ssmart_gw_policyrouting_script", "ssgw_egress_ifs", "sgw_egress_ifs",
  "sgw_egress_if", "ssmart_gw_egress_file", "ismart_gw_egress_file_period",
  "ssmart_gw_status_file", "ismart_gw_offset_tables",
  "ismart_gw_offset_rules", "bsmart_gw_allow_nat", "ismart_gw_period",
  "asmart_gw_stablecount", "asmart_gw_thresh",
  "asmart_gw_weight_exitlink_up", "asmart_gw_weight_exitlink_down",
  "asmart_gw_weight_etx", "asmart_gw_divider_etx", "ssmart_gw_uplink",
  "ismart_gw_speed", "bsmart_gw_uplink_nat", "ismart_gw_prefix",
  "bsrc_ip_routes", "amain_ip", "bset_ipforward", "plblock", "plparam",
  "vcomment", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-253)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -253,     1,  -253,   116,   117,   121,   122,   123,  -253,  -253,
     118,   124,   125,   126,   109,   112,   113,    96,    98,   103,
     105,   127,   131,   130,   132,   128,   133,   134,   136,   137,
     138,   141,   142,   143,   144,   145,   146,   147,   129,   148,
     151,   149,   167,   168,   171,   172,   174,   176,  -253,   177,
     179,   178,   180,   181,   182,   183,   185,   186,   187,   188,
     189,   190,   191,   197,   195,   198,    17,   204,   -95,   205,
    -253,  -253,  -253,   150,   213,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,   214,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,   215,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,   212,   108,  -253,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,   216,  -253,  -253,     4,    42,     2,  -253,  -253,
    -253,   220,  -253,     3,    76,  -253,  -253,    80,  -253,   110,
    -253,  -253,  -253,  -253,   114,  -253,  -253,  -253,  -253,    30,
      40,   221,  -253,  -253,  -253,  -253,  -253,  -253,  -253,   -98,
     217,    89,    89,    89,   119,   135,   139,   222,   223,   224,
     225,   226,   229,   230,   231,   233,   234,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,   236,  -253,  -253,  -253,
     237,  -253,   238,  -253,  -253,   111,  -253,   239,   240,   241,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,   227,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,   244,  -253,
    -253,   245,  -253,  -253,  -253,  -253,   246,  -253,  -253,  -253
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     1,     0,     0,     0,     0,     0,   120,   152,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   201,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     229,     4,     3,     0,     0,     5,     6,     7,     8,    90,
       9,    10,    11,    12,    13,    14,    15,    16,    19,    18,
      17,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    31,    33,    36,    37,    30,    32,    34,    38,    39,
      40,    41,    42,    43,    44,    65,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      60,    59,    61,    62,    63,    64,     0,    35,   144,   145,
      73,    66,    78,    67,   227,   154,   155,   156,   157,   158,
     159,   160,   161,   162,   163,   164,   165,   166,   173,   174,
     175,   170,   171,   172,   167,   168,   169,   176,    83,    68,
     146,   147,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   190,   191,   192,   188,   189,   193,   194,
     195,   196,   197,   198,   199,   200,   204,   205,   206,   207,
     208,   209,   210,   211,   212,   213,   214,   215,   216,   217,
     218,   220,     0,     0,   223,   224,   225,   226,    94,    70,
      94,    69,    89,   116,    71,     0,     0,     0,   203,   202,
     219,     0,   221,     0,     0,   153,    91,     0,    72,     0,
      74,    76,    75,    77,     0,    79,    81,    80,    82,     0,
       0,     0,    84,    86,    87,    88,    85,   222,    92,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    95,    97,    98,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,    96,    93,   115,     0,   117,   118,   119,
       0,   148,     0,   150,   122,     0,   121,     0,     0,     0,
     126,   127,   101,    99,   100,   128,   129,   131,   125,   132,
     133,   134,   135,   136,   137,   138,   139,   140,     0,   149,
     151,     0,   123,   142,   143,   141,     0,   228,   124,   130
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,    26,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -252,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,
    -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,  -253,
      -1
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,    71,    72,   141,   225,   240,   143,   226,   245,
     169,   227,   252,    73,   222,   219,   221,   233,   277,   224,
     237,   297,    74,   253,   254,   255,   278,   279,   312,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     291,   292,    75,    76,    77,    78,   241,   246,    79,   236,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   195,   229,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   298,
     293
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     137,     2,   307,   308,   309,   215,   216,   248,   258,   238,
     313,   314,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,   243,    38,   259,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,   294,   249,   250,   251,   295,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
     274,   275,   276,   155,   239,   158,    70,    70,    70,    70,
     161,   231,   164,   300,   331,   232,   149,   302,   213,   151,
     153,   303,   259,   138,   139,   140,   142,   145,   296,   144,
     304,   146,   147,   148,   167,   168,   170,   172,   185,   171,
     305,   173,   174,   244,   175,   176,   177,    70,   178,   179,
     180,   181,   183,   182,   218,   184,   186,   187,   188,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   275,   276,   189,   190,   191,   192,
     193,    70,   194,   196,   198,    70,   197,   199,   200,   311,
     202,   201,   203,   204,   205,   206,   207,   208,   209,   156,
     157,   159,   160,   210,   211,   212,   162,   163,   165,   166,
     301,   332,   150,   214,   217,   152,   154,   220,   223,   230,
     315,   228,   235,   310,   242,   247,   256,   257,   306,   318,
     336,   319,   320,   321,   322,   316,   299,   323,   324,   325,
     317,   326,   328,   327,   329,   330,   234,   333,   334,   335,
     337,     0,   338,   339
};

static const yytype_int16 yycheck[] =
{
       1,     0,   100,   101,   102,   100,   101,     5,     5,     5,
     262,   263,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,     5,    47,    46,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,     5,    80,    81,    82,     5,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,     7,   100,     7,   105,   105,   105,   105,
       7,     3,     7,     3,     3,     7,     7,     3,   101,     7,
       7,     7,    46,     7,     7,     4,     4,     9,    48,     6,
     100,     7,     7,     7,     7,     4,     6,     9,     9,     7,
     100,     8,     8,   101,     8,     8,     8,   105,     7,     7,
       7,     7,     6,     8,     4,     8,     8,     6,     9,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,     9,     9,     7,     7,
       6,   105,     6,     6,     6,   105,     7,     7,     7,   100,
       7,     9,     7,     7,     7,     7,     7,     7,     7,   103,
     104,   103,   104,     6,     9,     7,   103,   104,   103,   104,
     100,   100,   103,     9,     9,   103,   103,     4,     4,     7,
     101,     6,     6,     6,   225,   226,   227,     7,     7,     7,
       3,     8,     8,     8,     8,   100,   237,     8,     8,     8,
     101,     8,     6,     9,     7,     7,   220,     8,     8,     8,
       6,    -1,     7,     7
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   107,     0,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    47,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
     105,   108,   109,   119,   128,   148,   149,   150,   151,   154,
     156,   157,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   189,   190,   191,   194,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,   205,   206,   207,
     208,   209,   210,   211,   212,   213,   214,   216,     7,     7,
       4,   110,     4,   113,     6,     9,     7,     7,     7,     7,
     103,     7,   103,     7,   103,     7,   103,   104,     7,   103,
     104,     7,   103,   104,     7,   103,   104,     7,     4,   116,
       6,     7,     9,     8,     8,     8,     8,     8,     7,     7,
       7,     7,     8,     6,     8,     9,     8,     6,     9,     9,
       9,     7,     7,     6,     6,   192,     6,     7,     6,     7,
       7,     9,     7,     7,     7,     7,     7,     7,     7,     7,
       6,     9,     7,   101,     9,   100,   101,     9,     4,   121,
       4,   122,   120,     4,   125,   111,   114,   117,     6,   193,
       7,     3,     7,   123,   123,     6,   155,   126,     5,   100,
     112,   152,   216,     5,   101,   115,   153,   216,     5,    80,
      81,    82,   118,   129,   130,   131,   216,     7,     5,    46,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   124,   132,   133,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   216,     5,     5,    48,   127,   215,   216,
       3,   100,     3,     7,   100,   100,     7,   100,   101,   102,
       6,   100,   134,   134,   134,   101,   100,   101,     7,     8,
       8,     8,     8,     8,     8,     8,     8,     9,     6,     7,
       7,     3,   100,     8,     8,     8,     3,     6,     7,     7
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,   106,   107,   107,   107,   108,   108,   108,   108,   108,
     108,   108,   108,   108,   108,   108,   108,   108,   108,   108,
     108,   108,   108,   108,   108,   108,   108,   108,   108,   108,
     108,   108,   108,   108,   108,   108,   108,   108,   108,   108,
     108,   108,   108,   108,   108,   108,   108,   108,   108,   108,
     108,   108,   108,   108,   108,   108,   108,   108,   108,   108,
     108,   108,   108,   108,   108,   108,   109,   109,   109,   109,
     109,   109,   110,   111,   111,   112,   112,   113,   114,   114,
     115,   115,   116,   117,   117,   118,   118,   118,   118,   119,
     120,   120,   121,   122,   123,   123,   124,   124,   124,   124,
     124,   124,   124,   124,   124,   124,   124,   124,   124,   124,
     124,   124,   124,   124,   124,   125,   126,   126,   127,   127,
     128,   129,   130,   131,   131,   132,   133,   134,   135,   136,
     137,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   147,   147,   148,   149,   150,   151,   152,   152,
     153,   153,   154,   155,   156,   157,   158,   159,   160,   160,
     161,   161,   162,   162,   163,   163,   163,   164,   164,   164,
     165,   165,   165,   166,   166,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,   192,   192,   193,   194,   195,   196,   197,   198,   199,
     200,   201,   202,   203,   204,   205,   206,   206,   207,   208,
     209,   210,   210,   211,   212,   212,   213,   214,   215,   216
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     2,     2,     2,
       2,     2,     3,     0,     2,     1,     1,     3,     0,     2,
       1,     1,     3,     0,     2,     1,     1,     1,     1,     2,
       0,     2,     3,     3,     0,     2,     1,     1,     1,     2,
       2,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     3,     0,     2,     1,     1,
       1,     2,     2,     3,     4,     2,     2,     1,     2,     2,
       4,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     3,     3,     3,     2,     2,     2,     2,     2,     3,
       2,     3,     1,     1,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     0,     2,     1,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     3,
       2,     3,     4,     2,     2,     2,     2,     2,     3,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 93: /* ifdbody: TOK_OPEN ifstmts TOK_CLOSE  */
#line 404 "src/cfgparser/oparse.y"
{
  struct olsr_if *in = olsr_cnf->interfaces;
  printf("\nInterface Defaults");
  /*remove Interface Defaults from Interface list as they are no interface!*/
  olsr_cnf->interfaces = in->next;
  ifs_in_curr_cfg=0;
  /*free interface but keep its config intact?*/
  free(in->cnfi);
  free(in);

}
#line 1749 "src/cfgparser/oparse.c"
    break;

  case 120: /* ifdblock: TOK_INTERFACE_DEFAULTS  */
#line 452 "src/cfgparser/oparse.y"
{
  struct olsr_if *in = malloc(sizeof(*in));

  if (in == NULL) {
    fprintf(stderr, "Out of memory(ADD IF)\n");
    YYABORT;
  }

  in->cnf = get_default_if_config();
  in->cnfi = get_default_if_config();

  if (in->cnf == NULL || in->cnfi == NULL) {
    fprintf(stderr, "Out of memory(ADD DEFIFRULE)\n");
    if (in->cnf) {
      free(in->cnf);
    }
    if (in->cnfi) {
      free(in->cnfi);
    }
    free(in);
    YYABORT;
  }

  //should not need a name any more, as we free it on "}" again
  //in->name = strdup(interface_defaults_name);

  olsr_cnf->interface_defaults = in->cnf;

  /* Queue */
  in->next = olsr_cnf->interfaces;
  olsr_cnf->interfaces = in;
  ifs_in_curr_cfg=1;
  
  fflush(stdout);
}
#line 1789 "src/cfgparser/oparse.c"
    break;

  case 121: /* imaxipc: TOK_MAXIPC TOK_INTEGER  */
#line 490 "src/cfgparser/oparse.y"
{
  olsr_cnf->ipc_connections = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 1798 "src/cfgparser/oparse.c"
    break;

  case 122: /* ipchost: TOK_HOSTLABEL TOK_IPV4_ADDR  */
#line 497 "src/cfgparser/oparse.y"
{
  union olsr_ip_addr ipaddr;
  PARSER_DEBUG_PRINTF("\tIPC host: %s\n", yyvsp[0]->string);
  
  if (inet_pton(AF_INET, yyvsp[0]->string, &ipaddr.v4) == 0) {
    fprintf(stderr, "Failed converting IP address IPC %s\n", yyvsp[0]->string);
    YYABORT;
  }

  ip_prefix_list_add(&olsr_cnf->ipc_nets, &ipaddr, olsr_cnf->maxplen);

  free(yyvsp[0]->string);
  free(yyvsp[0]);
}
#line 1817 "src/cfgparser/oparse.c"
    break;

  case 123: /* ipcnet: TOK_NETLABEL TOK_IPV4_ADDR TOK_IPV4_ADDR  */
#line 514 "src/cfgparser/oparse.y"
{
  union olsr_ip_addr ipaddr, netmask;

  PARSER_DEBUG_PRINTF("\tIPC net: %s/%s\n", yyvsp[-1]->string, yyvsp[0]->string);
  
  if (inet_pton(AF_INET, yyvsp[-1]->string, &ipaddr.v4) == 0) {
    fprintf(stderr, "Failed converting IP net IPC %s\n", yyvsp[-1]->string);
    YYABORT;
  }

  if (inet_pton(AF_INET, yyvsp[0]->string, &netmask.v4) == 0) {
    fprintf(stderr, "Failed converting IP mask IPC %s\n", yyvsp[0]->string);
    YYABORT;
  }

  ip_prefix_list_add(&olsr_cnf->ipc_nets, &ipaddr, olsr_netmask_to_prefix(&netmask));

  free(yyvsp[-1]->string);
  free(yyvsp[-1]);
  free(yyvsp[0]->string);
  free(yyvsp[0]);
}
#line 1844 "src/cfgparser/oparse.c"
    break;

  case 124: /* ipcnet: TOK_NETLABEL TOK_IPV4_ADDR TOK_SLASH TOK_INTEGER  */
#line 537 "src/cfgparser/oparse.y"
{
  union olsr_ip_addr ipaddr;

  PARSER_DEBUG_PRINTF("\tIPC net: %s/%s\n", yyvsp[-2]->string, yyvsp[-1]->string);
  
  if (inet_pton(AF_INET, yyvsp[-2]->string, &ipaddr.v4) == 0) {
    fprintf(stderr, "Failed converting IP net IPC %s\n", yyvsp[-2]->string);
    YYABORT;
  }

  if (yyvsp[0]->integer > olsr_cnf->maxplen) {
    fprintf(stderr, "ipcnet: Prefix len %u > %d is not allowed!\n", yyvsp[0]->integer, olsr_cnf->maxplen);
    YYABORT;
  }

  ip_prefix_list_add(&olsr_cnf->ipc_nets, &ipaddr, yyvsp[0]->integer);

  free(yyvsp[-2]->string);
  free(yyvsp[-2]);
  free(yyvsp[0]);
}
#line 1870 "src/cfgparser/oparse.c"
    break;

  case 125: /* iifweight: TOK_IFWEIGHT TOK_INTEGER  */
#line 561 "src/cfgparser/oparse.y"
{
  int ifcnt = ifs_in_curr_cfg;
  struct olsr_if *ifs = olsr_cnf->interfaces;

  PARSER_DEBUG_PRINTF("Fixed willingness: %d\n", yyvsp[0]->integer);

  while (ifcnt) {
    ifs->cnf->weight.value = yyvsp[0]->integer;
    ifs->cnf->weight.fixed = true;
    ifs->cnfi->weight.value = yyvsp[0]->integer;
    ifs->cnfi->weight.fixed = true;

    ifs = ifs->next;
    ifcnt--;
  }

  free(yyvsp[0]);
}
#line 1893 "src/cfgparser/oparse.c"
    break;

  case 126: /* isetifmode: TOK_IFMODE TOK_STRING  */
#line 582 "src/cfgparser/oparse.y"
{
  int ifcnt = ifs_in_curr_cfg;
  struct olsr_if *ifs = olsr_cnf->interfaces;
	int mode = (strcmp(yyvsp[0]->string, "ether") == 0)?IF_MODE_ETHER:((strcmp(yyvsp[0]->string, "silent") == 0)?IF_MODE_SILENT:IF_MODE_MESH);

  PARSER_DEBUG_PRINTF("\tMode: %s\n", yyvsp[0]->string);

	SET_IFS_CONF(ifs, ifcnt, mode, mode);
	
  free(yyvsp[0]->string);
  free(yyvsp[0]);
}
#line 1910 "src/cfgparser/oparse.c"
    break;

  case 127: /* isetipv4mc: TOK_IPV4_ADDR  */
#line 598 "src/cfgparser/oparse.y"
{
  struct in_addr in;
  int ifcnt = ifs_in_curr_cfg;
  struct olsr_if *ifs = olsr_cnf->interfaces;

  PARSER_DEBUG_PRINTF("\tIPv4 broadcast: %s\n", yyvsp[0]->string);

  if (inet_pton(AF_INET, yyvsp[0]->string, &in) == 0) {
    fprintf(stderr, "isetipv4br: Failed converting IP address %s\n", yyvsp[0]->string);
    YYABORT;
  }

	SET_IFS_CONF(ifs, ifcnt, ipv4_multicast.v4, in);

  free(yyvsp[0]->string);
  free(yyvsp[0]);
}
#line 1932 "src/cfgparser/oparse.c"
    break;

  case 128: /* isetipv6mc: TOK_IPV6MULTICAST TOK_IPV6_ADDR  */
#line 618 "src/cfgparser/oparse.y"
{
  struct in6_addr in6;
  int ifcnt = ifs_in_curr_cfg;
  struct olsr_if *ifs = olsr_cnf->interfaces;

  PARSER_DEBUG_PRINTF("\tIPv6 multicast: %s\n", yyvsp[0]->string);

  if (inet_pton(AF_INET6, yyvsp[0]->string, &in6) <= 0) {
    fprintf(stderr, "isetipv6mc: Failed converting IP address %s\n", yyvsp[0]->string);
    YYABORT;
  }

	SET_IFS_CONF(ifs, ifcnt, ipv6_multicast.v6, in6);

  free(yyvsp[0]->string);
  free(yyvsp[0]);
}
#line 1954 "src/cfgparser/oparse.c"
    break;

  case 129: /* isetipv4src: TOK_IPV4SRC TOK_IPV4_ADDR  */
#line 638 "src/cfgparser/oparse.y"
{
  struct in_addr in;
  int ifcnt = ifs_in_curr_cfg;
  struct olsr_if *ifs = olsr_cnf->interfaces;

  PARSER_DEBUG_PRINTF("\tIPv4 src: %s\n", yyvsp[0]->string);

  if (inet_pton(AF_INET, yyvsp[0]->string, &in) == 0) {
    fprintf(stderr, "isetipv4src: Failed converting IP address %s\n", yyvsp[0]->string);
    YYABORT;
  }

	SET_IFS_CONF(ifs, ifcnt, ipv4_src.v4, in);

  free(yyvsp[0]->string);
  free(yyvsp[0]);
}
#line 1976 "src/cfgparser/oparse.c"
    break;

  case 130: /* isetipv6src: TOK_IPV6SRC TOK_IPV6_ADDR TOK_SLASH TOK_INTEGER  */
#line 658 "src/cfgparser/oparse.y"
{
  struct olsr_ip_prefix pr6;
  int ifcnt = ifs_in_curr_cfg;
  struct olsr_if *ifs = olsr_cnf->interfaces;

  PARSER_DEBUG_PRINTF("\tIPv6 src prefix: %s/%d\n", yyvsp[-2]->string, yyvsp[0]->integer);

  if (inet_pton(AF_INET6, yyvsp[-2]->string, &pr6.prefix.v6) <= 0) {
    fprintf(stderr, "isetipv6src: Failed converting IP address %s\n", yyvsp[-2]->string);
    YYABORT;
  }
  if (yyvsp[0]->integer > 128) {
    fprintf(stderr, "isetipv6src: Illegal Prefixlength %d\n", yyvsp[0]->integer);
    YYABORT;
  }
  pr6.prefix_len = yyvsp[0]->integer;

	SET_IFS_CONF(ifs, ifcnt, ipv6_src, pr6);

  free(yyvsp[-2]->string);
  free(yyvsp[-2]);
}
#line 2003 "src/cfgparser/oparse.c"
    break;

  case 131: /* isetipv6src: TOK_IPV6SRC TOK_IPV6_ADDR  */
#line 681 "src/cfgparser/oparse.y"
{
  struct olsr_ip_prefix pr6;
  int ifcnt = ifs_in_curr_cfg;
  struct olsr_if *ifs = olsr_cnf->interfaces;

  PARSER_DEBUG_PRINTF("\tIPv6 src prefix: %s/%d\n", yyvsp[0]->string, 128);

  if (inet_pton(AF_INET6, yyvsp[0]->string, &pr6.prefix.v6) <= 0) {
    fprintf(stderr, "isetipv6src: Failed converting IP address %s\n", yyvsp[0]->string);
    YYABORT;
  }
  pr6.prefix_len = 128;

  SET_IFS_CONF(ifs, ifcnt, ipv6_src, pr6);

  free(yyvsp[0]->string);
  free(yyvsp[0]);
}
#line 2026 "src/cfgparser/oparse.c"
    break;

  case 132: /* isethelloint: TOK_HELLOINT TOK_FLOAT  */
#line 702 "src/cfgparser/oparse.y"
{
  int ifcnt = ifs_in_curr_cfg;
  struct olsr_if *ifs = olsr_cnf->interfaces;

  PARSER_DEBUG_PRINTF("\tHELLO interval: %0.2f\n", (double)yyvsp[0]->floating);

	SET_IFS_CONF(ifs, ifcnt, hello_params.emission_interval, yyvsp[0]->floating);

  free(yyvsp[0]);
}
#line 2041 "src/cfgparser/oparse.c"
    break;

  case 133: /* isethelloval: TOK_HELLOVAL TOK_FLOAT  */
#line 714 "src/cfgparser/oparse.y"
{
  int ifcnt = ifs_in_curr_cfg;
  struct olsr_if *ifs = olsr_cnf->interfaces;

  PARSER_DEBUG_PRINTF("\tHELLO validity: %0.2f\n", (double)yyvsp[0]->floating);

	SET_IFS_CONF(ifs, ifcnt, hello_params.validity_time, yyvsp[0]->floating);

  free(yyvsp[0]);
}
#line 2056 "src/cfgparser/oparse.c"
    break;

  case 134: /* isettcint: TOK_TCINT TOK_FLOAT  */
#line 726 "src/cfgparser/oparse.y"
{
  int ifcnt = ifs_in_curr_cfg;
  struct olsr_if *ifs = olsr_cnf->interfaces;

  PARSER_DEBUG_PRINTF("\tTC interval: %0.2f\n", (double)yyvsp[0]->floating);

	SET_IFS_CONF(ifs, ifcnt, tc_params.emission_interval, yyvsp[0]->floating);

  free(yyvsp[0]);
}
#line 2071 "src/cfgparser/oparse.c"
    break;

  case 135: /* isettcval: TOK_TCVAL TOK_FLOAT  */
#line 738 "src/cfgparser/oparse.y"
{
  int ifcnt = ifs_in_curr_cfg;
  struct olsr_if *ifs = olsr_cnf->interfaces;
  
  PARSER_DEBUG_PRINTF("\tTC validity: %0.2f\n", (double)yyvsp[0]->floating);
  
 SET_IFS_CONF(ifs, ifcnt, tc_params.validity_time, yyvsp[0]->floating);

  free(yyvsp[0]);
}
#line 2086 "src/cfgparser/oparse.c"
    break;

  case 136: /* isetmidint: TOK_MIDINT TOK_FLOAT  */
#line 750 "src/cfgparser/oparse.y"
{
  int ifcnt = ifs_in_curr_cfg;
  struct olsr_if *ifs = olsr_cnf->interfaces;


  PARSER_DEBUG_PRINTF("\tMID interval: %0.2f\n", (double)yyvsp[0]->floating);
  
  SET_IFS_CONF(ifs, ifcnt, mid_params.emission_interval, yyvsp[0]->floating);

  free(yyvsp[0]);
}
#line 2102 "src/cfgparser/oparse.c"
    break;

  case 137: /* isetmidval: TOK_MIDVAL TOK_FLOAT  */
#line 763 "src/cfgparser/oparse.y"
{
  int ifcnt = ifs_in_curr_cfg;
  struct olsr_if *ifs = olsr_cnf->interfaces;

  PARSER_DEBUG_PRINTF("\tMID validity: %0.2f\n", (double)yyvsp[0]->floating);
  
  SET_IFS_CONF(ifs, ifcnt, mid_params.validity_time, yyvsp[0]->floating);

  free(yyvsp[0]);
}
#line 2117 "src/cfgparser/oparse.c"
    break;

  case 138: /* isethnaint: TOK_HNAINT TOK_FLOAT  */
#line 775 "src/cfgparser/oparse.y"
{
  int ifcnt = ifs_in_curr_cfg;
  struct olsr_if *ifs = olsr_cnf->interfaces;
  
  PARSER_DEBUG_PRINTF("\tHNA interval: %0.2f\n", (double)yyvsp[0]->floating);

  SET_IFS_CONF(ifs, ifcnt, hna_params.emission_interval, yyvsp[0]->floating);

  free(yyvsp[0]);
}
#line 2132 "src/cfgparser/oparse.c"
    break;

  case 139: /* isethnaval: TOK_HNAVAL TOK_FLOAT  */
#line 787 "src/cfgparser/oparse.y"
{
  int ifcnt = ifs_in_curr_cfg;
  struct olsr_if *ifs = olsr_cnf->interfaces;

  PARSER_DEBUG_PRINTF("\tHNA validity: %0.2f\n", (double)yyvsp[0]->floating);

  SET_IFS_CONF(ifs, ifcnt, hna_params.validity_time, yyvsp[0]->floating);

  free(yyvsp[0]);
}
#line 2147 "src/cfgparser/oparse.c"
    break;

  case 140: /* isetautodetchg: TOK_AUTODETCHG TOK_BOOLEAN  */
#line 799 "src/cfgparser/oparse.y"
{
  int ifcnt = ifs_in_curr_cfg;
  struct olsr_if *ifs = olsr_cnf->interfaces;

  PARSER_DEBUG_PRINTF("\tAutodetect changes: %s\n", yyvsp[0]->boolean ? "YES" : "NO");

  SET_IFS_CONF(ifs, ifcnt, autodetect_chg, yyvsp[0]->boolean);

  free(yyvsp[0]);
}
#line 2162 "src/cfgparser/oparse.c"
    break;

  case 141: /* isetlqmult: TOK_LQ_MULT TOK_DEFAULT TOK_FLOAT  */
#line 812 "src/cfgparser/oparse.y"
{
  if (lq_mult_helper(yyvsp[-1], yyvsp[0]) < 0) {
    YYABORT;
  }
}
#line 2172 "src/cfgparser/oparse.c"
    break;

  case 142: /* isetlqmult: TOK_LQ_MULT TOK_IPV4_ADDR TOK_FLOAT  */
#line 819 "src/cfgparser/oparse.y"
{
  if (lq_mult_helper(yyvsp[-1], yyvsp[0]) < 0) {
    YYABORT;
  }
}
#line 2182 "src/cfgparser/oparse.c"
    break;

  case 143: /* isetlqmult: TOK_LQ_MULT TOK_IPV6_ADDR TOK_FLOAT  */
#line 826 "src/cfgparser/oparse.y"
{
  if (lq_mult_helper(yyvsp[-1], yyvsp[0]) < 0) {
    YYABORT;
  }
}
#line 2192 "src/cfgparser/oparse.c"
    break;

  case 144: /* idebug: TOK_DEBUGLEVEL TOK_INTEGER  */
#line 834 "src/cfgparser/oparse.y"
{
  olsr_cnf->debug_level = yyvsp[0]->integer;
  PARSER_DEBUG_PRINTF("Debug level: %d\n", olsr_cnf->debug_level);
  free(yyvsp[0]);
}
#line 2202 "src/cfgparser/oparse.c"
    break;

  case 145: /* iipversion: TOK_IPVERSION TOK_INTEGER  */
#line 843 "src/cfgparser/oparse.y"
{
  if (yyvsp[0]->integer == 4) {
    olsr_cnf->ip_version = AF_INET;
    olsr_cnf->ipsize = sizeof(struct in_addr);
    olsr_cnf->maxplen = 32;
  } else if (yyvsp[0]->integer == 6) {
    olsr_cnf->ip_version = AF_INET6;
    olsr_cnf->ipsize = sizeof(struct in6_addr);
    olsr_cnf->maxplen = 128;
  } else {
    fprintf(stderr, "IPversion must be 4 or 6!\n");
    YYABORT;
  }

  PARSER_DEBUG_PRINTF("IpVersion: %d\n", yyvsp[0]->integer);
  free(yyvsp[0]);
}
#line 2224 "src/cfgparser/oparse.c"
    break;

  case 146: /* fibmetric: TOK_FIBMETRIC TOK_STRING  */
#line 863 "src/cfgparser/oparse.y"
{
  int i;
  PARSER_DEBUG_PRINTF("FIBMetric: %s\n", yyvsp[0]->string);
  for (i=0; i<FIBM_CNT; i++) {
    if (strcmp(yyvsp[0]->string, FIB_METRIC_TXT[i]) == 0) {
      olsr_cnf->fib_metric = i;
      break;
    }
  }
  if (i == FIBM_CNT) {
    fprintf(stderr, "Bad FIBMetric value: %s\n", yyvsp[0]->string);
    YYABORT;
  }
  free(yyvsp[-1]);
  free(yyvsp[0]->string);
  free(yyvsp[0]);
}
#line 2246 "src/cfgparser/oparse.c"
    break;

  case 147: /* afibmetricdefault: TOK_FIBMETRICDEFAULT TOK_INTEGER  */
#line 883 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("FIBMetricDefault: %d\n", yyvsp[0]->integer);
  olsr_cnf->fib_metric_default = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 2256 "src/cfgparser/oparse.c"
    break;

  case 148: /* ihna4entry: TOK_IPV4_ADDR TOK_IPV4_ADDR  */
#line 891 "src/cfgparser/oparse.y"
{
  union olsr_ip_addr ipaddr, netmask;

  if (olsr_cnf->ip_version == AF_INET6) {
    fprintf(stderr, "IPv4 addresses can only be used if \"IpVersion\" == 4, skipping HNA.\n");
    olsr_startup_sleep(3);
  }
  else {
    PARSER_DEBUG_PRINTF("HNA IPv4 entry: %s/%s\n", yyvsp[-1]->string, yyvsp[0]->string);

    if (inet_pton(AF_INET, yyvsp[-1]->string, &ipaddr.v4) <= 0) {
      fprintf(stderr, "ihna4entry: Failed converting IP address %s\n", yyvsp[-1]->string);
      YYABORT;
    }
    if (inet_pton(AF_INET, yyvsp[0]->string, &netmask.v4) <= 0) {
      fprintf(stderr, "ihna4entry: Failed converting IP address %s\n", yyvsp[-1]->string);
      YYABORT;
    }

    /* check that the given IP address is actually a network address */
    if ((ipaddr.v4.s_addr & ~netmask.v4.s_addr) != 0) {
      fprintf(stderr, "ihna4entry: The ipaddress \"%s\" is not a network address!\n", yyvsp[-1]->string);
      YYABORT;
    }

    /* Queue */
    ip_prefix_list_add(&olsr_cnf->hna_entries, &ipaddr, olsr_netmask_to_prefix(&netmask));
  }
  free(yyvsp[-1]->string);
  free(yyvsp[-1]);
  free(yyvsp[0]->string);
  free(yyvsp[0]);
}
#line 2294 "src/cfgparser/oparse.c"
    break;

  case 149: /* ihna4entry: TOK_IPV4_ADDR TOK_SLASH TOK_INTEGER  */
#line 925 "src/cfgparser/oparse.y"
{
  union olsr_ip_addr ipaddr, netmask;

  if (olsr_cnf->ip_version == AF_INET6) {
    fprintf(stderr, "IPv4 addresses can only be used if \"IpVersion\" == 4, skipping HNA.\n");
    olsr_startup_sleep(3);
  }
  else {
    PARSER_DEBUG_PRINTF("HNA IPv4 entry: %s/%d\n", yyvsp[-2]->string, yyvsp[0]->integer);

    if (inet_pton(AF_INET, yyvsp[-2]->string, &ipaddr.v4) <= 0) {
      fprintf(stderr, "ihna4entry: Failed converting IP address %s\n", yyvsp[-2]->string);
      YYABORT;
    }
    if (yyvsp[0]->integer > olsr_cnf->maxplen) {
      fprintf(stderr, "ihna4entry: Prefix len %u > %d is not allowed!\n", yyvsp[0]->integer, olsr_cnf->maxplen);
      YYABORT;
    }

    /* check that the given IP address is actually a network address */
    olsr_prefix_to_netmask(&netmask, yyvsp[0]->integer);
    if ((ipaddr.v4.s_addr & ~netmask.v4.s_addr) != 0) {
      fprintf(stderr, "ihna4entry: The ipaddress \"%s\" is not a network address!\n", yyvsp[-2]->string);
      YYABORT;
    }

    /* Queue */
    ip_prefix_list_add(&olsr_cnf->hna_entries, &ipaddr, yyvsp[0]->integer);
  }
  free(yyvsp[-2]->string);
  free(yyvsp[-2]);
  free(yyvsp[0]);
}
#line 2332 "src/cfgparser/oparse.c"
    break;

  case 150: /* ihna6entry: TOK_IPV6_ADDR TOK_INTEGER  */
#line 961 "src/cfgparser/oparse.y"
{
  if (add_ipv6_addr(yyvsp[-1], yyvsp[0])) {
    YYABORT;
  }
}
#line 2342 "src/cfgparser/oparse.c"
    break;

  case 151: /* ihna6entry: TOK_IPV6_ADDR TOK_SLASH TOK_INTEGER  */
#line 967 "src/cfgparser/oparse.y"
{
  if (add_ipv6_addr(yyvsp[-2], yyvsp[0])) {
    YYABORT;
  }
}
#line 2352 "src/cfgparser/oparse.c"
    break;

  case 152: /* ifstart: TOK_INTERFACE  */
#line 975 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("setting ifs_in_curr_cfg = 0\n");
  ifs_in_curr_cfg = 0;
}
#line 2361 "src/cfgparser/oparse.c"
    break;

  case 153: /* ifnick: TOK_STRING  */
#line 982 "src/cfgparser/oparse.y"
{
  struct olsr_if *in, *last;
  in = olsr_cnf->interfaces;
  last = NULL;
  while (in != NULL) {
    if (strcmp(in->name, yyvsp[0]->string) == 0) {
      free (yyvsp[0]->string);
      break;
    }
    last = in;
    in = in->next;
  }

  if (in != NULL) {
    /* remove old interface from list to add it later at the beginning */
    if (last) {
      last->next = in->next;
    }
    else {
      olsr_cnf->interfaces = in->next;
    }
  }
  else {
    in = malloc(sizeof(*in));
    if (in == NULL) {
      fprintf(stderr, "Out of memory(ADD IF)\n");
      YYABORT;
    }
    memset(in, 0, sizeof(*in));

    in->cnf = malloc(sizeof(*in->cnf));
    if (in->cnf == NULL) {
      fprintf(stderr, "Out of memory(ADD IFRULE)\n");
      free(in);
      YYABORT;
    }
    memset(in->cnf, 0x00, sizeof(*in->cnf));

    in->cnfi = malloc(sizeof(*in->cnfi));
    if (in->cnfi == NULL) {
      fprintf(stderr, "Out of memory(ADD IFRULE)\n");
      free (in->cnf);
      free(in);
      YYABORT;
    }
    memset(in->cnfi, 0xFF, sizeof(*in->cnfi));
    in->cnfi->orig_lq_mult_cnt=0;

    in->name = yyvsp[0]->string;
  }
  /* Queue */
  in->next = olsr_cnf->interfaces;
  olsr_cnf->interfaces = in;
  ifs_in_curr_cfg++;
  free(yyvsp[0]);
}
#line 2422 "src/cfgparser/oparse.c"
    break;

  case 154: /* bnoint: TOK_NOINT TOK_BOOLEAN  */
#line 1041 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Noint set to %d\n", yyvsp[0]->boolean);
  olsr_cnf->allow_no_interfaces = yyvsp[0]->boolean;
  free(yyvsp[0]);
}
#line 2432 "src/cfgparser/oparse.c"
    break;

  case 155: /* atos: TOK_TOS TOK_INTEGER  */
#line 1049 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("TOS: %d\n", yyvsp[0]->integer);
  olsr_cnf->tos = yyvsp[0]->integer;
  free(yyvsp[0]);

}
#line 2443 "src/cfgparser/oparse.c"
    break;

  case 156: /* aolsrport: TOK_OLSRPORT TOK_INTEGER  */
#line 1058 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("OlsrPort: %d\n", yyvsp[0]->integer);
  olsr_cnf->olsrport = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 2453 "src/cfgparser/oparse.c"
    break;

  case 157: /* irtproto: TOK_RTPROTO TOK_INTEGER  */
#line 1066 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("RtProto: %d\n", yyvsp[0]->integer);
  olsr_cnf->rt_proto = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 2463 "src/cfgparser/oparse.c"
    break;

  case 158: /* irttable: TOK_RTTABLE TOK_INTEGER  */
#line 1074 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("RtTable: %d\n", yyvsp[0]->integer);
  olsr_cnf->rt_table = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 2473 "src/cfgparser/oparse.c"
    break;

  case 159: /* irttable: TOK_RTTABLE TOK_AUTO  */
#line 1080 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("RtTable: auto\n");
  olsr_cnf->rt_table = DEF_RT_AUTO;
  free(yyvsp[0]);
}
#line 2483 "src/cfgparser/oparse.c"
    break;

  case 160: /* irttable_default: TOK_RTTABLE_DEFAULT TOK_INTEGER  */
#line 1088 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("RtTableDefault: %d\n", yyvsp[0]->integer);
  olsr_cnf->rt_table_default = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 2493 "src/cfgparser/oparse.c"
    break;

  case 161: /* irttable_default: TOK_RTTABLE_DEFAULT TOK_AUTO  */
#line 1094 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("RtTableDefault: auto\n");
  olsr_cnf->rt_table_default = DEF_RT_AUTO;
  free(yyvsp[0]);
}
#line 2503 "src/cfgparser/oparse.c"
    break;

  case 162: /* irttable_tunnel: TOK_RTTABLE_TUNNEL TOK_INTEGER  */
#line 1102 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("RtTableTunnel: %d\n", yyvsp[0]->integer);
  olsr_cnf->rt_table_tunnel = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 2513 "src/cfgparser/oparse.c"
    break;

  case 163: /* irttable_tunnel: TOK_RTTABLE_TUNNEL TOK_AUTO  */
#line 1108 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("RtTableTunnel: auto\n");
  olsr_cnf->rt_table_tunnel = DEF_RT_AUTO;
  free(yyvsp[0]);
}
#line 2523 "src/cfgparser/oparse.c"
    break;

  case 164: /* irttable_priority: TOK_RTTABLE_PRIORITY TOK_INTEGER  */
#line 1116 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("RtTablePriority: %d\n", yyvsp[0]->integer);
  olsr_cnf->rt_table_pri = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 2533 "src/cfgparser/oparse.c"
    break;

  case 165: /* irttable_priority: TOK_RTTABLE_PRIORITY TOK_AUTO  */
#line 1122 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("RtTablePriority: auto\n");
  olsr_cnf->rt_table_pri = DEF_RT_AUTO;
  free(yyvsp[0]);
}
#line 2543 "src/cfgparser/oparse.c"
    break;

  case 166: /* irttable_priority: TOK_RTTABLE_PRIORITY TOK_NONE  */
#line 1128 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("RtTablePriority: none\n");
  olsr_cnf->rt_table_pri = DEF_RT_NONE;
  free(yyvsp[0]);
}
#line 2553 "src/cfgparser/oparse.c"
    break;

  case 167: /* irttable_default_priority: TOK_RTTABLE_DEFAULT_PRIORITY TOK_INTEGER  */
#line 1136 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("RtTableDefaultPriority: %d\n", yyvsp[0]->integer);
  olsr_cnf->rt_table_default_pri = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 2563 "src/cfgparser/oparse.c"
    break;

  case 168: /* irttable_default_priority: TOK_RTTABLE_DEFAULT_PRIORITY TOK_AUTO  */
#line 1142 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("RtTableDefaultPriority: auto\n");
  olsr_cnf->rt_table_default_pri = DEF_RT_AUTO;
  free(yyvsp[0]);
}
#line 2573 "src/cfgparser/oparse.c"
    break;

  case 169: /* irttable_default_priority: TOK_RTTABLE_DEFAULT_PRIORITY TOK_NONE  */
#line 1148 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("RtTableDefaultPriority: none\n");
  olsr_cnf->rt_table_default_pri = DEF_RT_NONE;
  free(yyvsp[0]);
}
#line 2583 "src/cfgparser/oparse.c"
    break;

  case 170: /* irttable_tunnel_priority: TOK_RTTABLE_TUNNEL_PRIORITY TOK_INTEGER  */
#line 1156 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("RtTableTunnelPriority: %d\n", yyvsp[0]->integer);
  olsr_cnf->rt_table_tunnel_pri = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 2593 "src/cfgparser/oparse.c"
    break;

  case 171: /* irttable_tunnel_priority: TOK_RTTABLE_TUNNEL_PRIORITY TOK_AUTO  */
#line 1162 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("RtTableTunnelPriority: auto\n");
  olsr_cnf->rt_table_tunnel_pri = DEF_RT_AUTO;
  free(yyvsp[0]);
}
#line 2603 "src/cfgparser/oparse.c"
    break;

  case 172: /* irttable_tunnel_priority: TOK_RTTABLE_TUNNEL_PRIORITY TOK_NONE  */
#line 1168 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("RtTableTunnelPriority: none\n");
  olsr_cnf->rt_table_tunnel_pri = DEF_RT_NONE;
  free(yyvsp[0]);
}
#line 2613 "src/cfgparser/oparse.c"
    break;

  case 173: /* irttable_defaultolsr_priority: TOK_RTTABLE_DEFAULTOLSR_PRIORITY TOK_INTEGER  */
#line 1176 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("RtTableDefaultOlsrPriority: %d\n", yyvsp[0]->integer);
  olsr_cnf->rt_table_defaultolsr_pri = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 2623 "src/cfgparser/oparse.c"
    break;

  case 174: /* irttable_defaultolsr_priority: TOK_RTTABLE_DEFAULTOLSR_PRIORITY TOK_AUTO  */
#line 1182 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("RtTableDefaultOlsrPriority: auto\n");
  olsr_cnf->rt_table_defaultolsr_pri = DEF_RT_AUTO;
  free(yyvsp[0]);
}
#line 2633 "src/cfgparser/oparse.c"
    break;

  case 175: /* irttable_defaultolsr_priority: TOK_RTTABLE_DEFAULTOLSR_PRIORITY TOK_NONE  */
#line 1188 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("RtTableDefaultOlsrPriority: none\n");
  olsr_cnf->rt_table_defaultolsr_pri = DEF_RT_NONE;
  free(yyvsp[0]);
}
#line 2643 "src/cfgparser/oparse.c"
    break;

  case 176: /* awillingness: TOK_WILLINGNESS TOK_INTEGER  */
#line 1196 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Willingness: %d\n", yyvsp[0]->integer);
  olsr_cnf->willingness_auto = false;
  olsr_cnf->willingness = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 2654 "src/cfgparser/oparse.c"
    break;

  case 177: /* busehyst: TOK_USEHYST TOK_BOOLEAN  */
#line 1205 "src/cfgparser/oparse.y"
{
  olsr_cnf->use_hysteresis = yyvsp[0]->boolean;
  PARSER_DEBUG_PRINTF("Hysteresis %s\n", olsr_cnf->use_hysteresis ? "enabled" : "disabled");
  free(yyvsp[0]);
}
#line 2664 "src/cfgparser/oparse.c"
    break;

  case 178: /* fhystscale: TOK_HYSTSCALE TOK_FLOAT  */
#line 1213 "src/cfgparser/oparse.y"
{
  olsr_cnf->hysteresis_param.scaling = yyvsp[0]->floating;
  PARSER_DEBUG_PRINTF("Hysteresis Scaling: %0.2f\n", (double)yyvsp[0]->floating);
  free(yyvsp[0]);
}
#line 2674 "src/cfgparser/oparse.c"
    break;

  case 179: /* fhystupper: TOK_HYSTUPPER TOK_FLOAT  */
#line 1221 "src/cfgparser/oparse.y"
{
  olsr_cnf->hysteresis_param.thr_high = yyvsp[0]->floating;
  PARSER_DEBUG_PRINTF("Hysteresis UpperThr: %0.2f\n", (double)yyvsp[0]->floating);
  free(yyvsp[0]);
}
#line 2684 "src/cfgparser/oparse.c"
    break;

  case 180: /* fhystlower: TOK_HYSTLOWER TOK_FLOAT  */
#line 1229 "src/cfgparser/oparse.y"
{
  olsr_cnf->hysteresis_param.thr_low = yyvsp[0]->floating;
  PARSER_DEBUG_PRINTF("Hysteresis LowerThr: %0.2f\n", (double)yyvsp[0]->floating);
  free(yyvsp[0]);
}
#line 2694 "src/cfgparser/oparse.c"
    break;

  case 181: /* fpollrate: TOK_POLLRATE TOK_FLOAT  */
#line 1237 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Pollrate %0.2f\n", (double)yyvsp[0]->floating);
  olsr_cnf->pollrate = yyvsp[0]->floating;
  free(yyvsp[0]);
}
#line 2704 "src/cfgparser/oparse.c"
    break;

  case 182: /* fnicchgspollrt: TOK_NICCHGSPOLLRT TOK_FLOAT  */
#line 1245 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("NIC Changes Pollrate %0.2f\n", (double)yyvsp[0]->floating);
  olsr_cnf->nic_chgs_pollrate = yyvsp[0]->floating;
  free(yyvsp[0]);
}
#line 2714 "src/cfgparser/oparse.c"
    break;

  case 183: /* atcredundancy: TOK_TCREDUNDANCY TOK_INTEGER  */
#line 1253 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("TC redundancy %d\n", yyvsp[0]->integer);
  olsr_cnf->tc_redundancy = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 2724 "src/cfgparser/oparse.c"
    break;

  case 184: /* amprcoverage: TOK_MPRCOVERAGE TOK_INTEGER  */
#line 1261 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("MPR coverage %d\n", yyvsp[0]->integer);
  olsr_cnf->mpr_coverage = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 2734 "src/cfgparser/oparse.c"
    break;

  case 185: /* alq_level: TOK_LQ_LEVEL TOK_INTEGER  */
#line 1269 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Link quality level %d\n", yyvsp[0]->integer);
  olsr_cnf->lq_level = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 2744 "src/cfgparser/oparse.c"
    break;

  case 186: /* alq_fish: TOK_LQ_FISH TOK_INTEGER  */
#line 1277 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Link quality fish eye %d\n", yyvsp[0]->integer);
  olsr_cnf->lq_fish = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 2754 "src/cfgparser/oparse.c"
    break;

  case 187: /* alq_aging: TOK_LQ_AGING TOK_FLOAT  */
#line 1285 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Link quality aging factor %f\n", (double)yyvsp[0]->floating);
  olsr_cnf->lq_aging = yyvsp[0]->floating;
  free(yyvsp[0]);
}
#line 2764 "src/cfgparser/oparse.c"
    break;

  case 188: /* amin_tc_vtime: TOK_MIN_TC_VTIME TOK_FLOAT  */
#line 1293 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Minimum TC validity time %f\n", (double)yyvsp[0]->floating);
  olsr_cnf->min_tc_vtime = yyvsp[0]->floating;
  free(yyvsp[0]);
}
#line 2774 "src/cfgparser/oparse.c"
    break;

  case 189: /* alock_file: TOK_LOCK_FILE TOK_STRING  */
#line 1301 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Lock file %s\n", yyvsp[0]->string);
  if (olsr_cnf->lock_file) free(olsr_cnf->lock_file);
  olsr_cnf->lock_file = yyvsp[0]->string;
  free(yyvsp[0]);
}
#line 2785 "src/cfgparser/oparse.c"
    break;

  case 190: /* alq_plugin: TOK_LQ_PLUGIN TOK_STRING  */
#line 1309 "src/cfgparser/oparse.y"
{
  if (olsr_cnf->lq_algorithm) free(olsr_cnf->lq_algorithm);
  olsr_cnf->lq_algorithm = yyvsp[0]->string;
  PARSER_DEBUG_PRINTF("LQ Algorithm: %s\n", yyvsp[0]->string);
  free(yyvsp[0]);
}
#line 2796 "src/cfgparser/oparse.c"
    break;

  case 191: /* anat_thresh: TOK_LQ_NAT_THRESH TOK_FLOAT  */
#line 1318 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("NAT threshold %0.2f\n", (double)yyvsp[0]->floating);
  olsr_cnf->lq_nat_thresh = yyvsp[0]->floating;
  free(yyvsp[0]);
}
#line 2806 "src/cfgparser/oparse.c"
    break;

  case 192: /* bclear_screen: TOK_CLEAR_SCREEN TOK_BOOLEAN  */
#line 1326 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Clear screen %s\n", yyvsp[0]->boolean ? "enabled" : "disabled");
  olsr_cnf->clear_screen = yyvsp[0]->boolean;
  free(yyvsp[0]);
}
#line 2816 "src/cfgparser/oparse.c"
    break;

  case 193: /* suse_niit: TOK_USE_NIIT TOK_BOOLEAN  */
#line 1334 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Use NIIT ip translation: %s\n", yyvsp[0]->boolean ? "enabled" : "disabled");
  olsr_cnf->use_niit = yyvsp[0]->boolean;
  free(yyvsp[0]);
}
#line 2826 "src/cfgparser/oparse.c"
    break;

  case 194: /* bsmart_gw: TOK_SMART_GW TOK_BOOLEAN  */
#line 1342 "src/cfgparser/oparse.y"
{
	PARSER_DEBUG_PRINTF("Smart gateway system: %s\n", yyvsp[0]->boolean ? "enabled" : "disabled");
	olsr_cnf->smart_gw_active = yyvsp[0]->boolean;
	free(yyvsp[0]);
}
#line 2836 "src/cfgparser/oparse.c"
    break;

  case 195: /* bsmart_gw_always_remove_server_tunnel: TOK_SMART_GW_ALWAYS_REMOVE_SERVER_TUNNEL TOK_BOOLEAN  */
#line 1350 "src/cfgparser/oparse.y"
{
	PARSER_DEBUG_PRINTF("Smart gateway always remove server tunnel: %s\n", yyvsp[0]->boolean ? "enabled" : "disabled");
	olsr_cnf->smart_gw_always_remove_server_tunnel = yyvsp[0]->boolean;
	free(yyvsp[0]);
}
#line 2846 "src/cfgparser/oparse.c"
    break;

  case 196: /* ismart_gw_use_count: TOK_SMART_GW_USE_COUNT TOK_INTEGER  */
#line 1358 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Smart gateway use count: %d\n", yyvsp[0]->integer);
  olsr_cnf->smart_gw_use_count = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 2856 "src/cfgparser/oparse.c"
    break;

  case 197: /* ismart_gw_takedown_percentage: TOK_SMART_GW_TAKEDOWN_PERCENTAGE TOK_INTEGER  */
#line 1366 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Smart gateway takedown percentage: %d\n", yyvsp[0]->integer);
  olsr_cnf->smart_gw_takedown_percentage = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 2866 "src/cfgparser/oparse.c"
    break;

  case 198: /* ssmart_gw_instance_id: TOK_SMART_GW_INSTANCE_ID TOK_STRING  */
#line 1374 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Smart gateway instance id: %s\n", yyvsp[0]->string);
  if (olsr_cnf->smart_gw_instance_id) free(olsr_cnf->smart_gw_instance_id);
  olsr_cnf->smart_gw_instance_id = yyvsp[0]->string;
  free(yyvsp[0]);
}
#line 2877 "src/cfgparser/oparse.c"
    break;

  case 199: /* ssmart_gw_policyrouting_script: TOK_SMART_GW_POLICYROUTING_SCRIPT TOK_STRING  */
#line 1383 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Smart gateway policy routing script: %s\n", yyvsp[0]->string);
  if (olsr_cnf->smart_gw_policyrouting_script) free(olsr_cnf->smart_gw_policyrouting_script);
  olsr_cnf->smart_gw_policyrouting_script = yyvsp[0]->string;
  free(yyvsp[0]);
}
#line 2888 "src/cfgparser/oparse.c"
    break;

  case 203: /* sgw_egress_if: TOK_STRING  */
#line 1398 "src/cfgparser/oparse.y"
{
  struct sgw_egress_if *in, *previous, *last;
  char * str = yyvsp[0]->string;
  char *end;

  /* Trim leading space */
  while(isspace(*str)) {
    str++;
  }

  /* Trim trailing space */
  end = &str[strlen(str) - 1];
  while((end > str) && isspace(*end)) {
    end--;
  }

  /* Write new null terminator */
  end[1] = '\0';

  if(*str == '\0') {
    PARSER_DEBUG_PRINTF("Smart gateway egress interface: <empty> (skipped)\n");
  } else {
    PARSER_DEBUG_PRINTF("Smart gateway egress interface: %s\n", str);

    in = olsr_cnf->smart_gw_egress_interfaces;
    previous = NULL;
    while (in != NULL) {
      if (strcmp(in->name, str) == 0) {
        free (yyvsp[0]->string);
        break;
      }
      previous = in;
      in = in->next;
    }

    if (in != NULL) {
      /* remove old interface from list to add it later at the end */
      if (previous) {
        previous->next = in->next;
      }
      else {
        olsr_cnf->smart_gw_egress_interfaces = in->next;
      }
      in->next = NULL;
    }
    else {
      /* interface in not in the list: create a new entry to add it later at the end */
      in = malloc(sizeof(*in));
      if (in == NULL) {
        fprintf(stderr, "Out of memory(ADD IF)\n");
        YYABORT;
      }
      memset(in, 0, sizeof(*in));

      in->name = strdup(str);
      free (yyvsp[0]->string);
    }

    last = olsr_cnf->smart_gw_egress_interfaces;
    while (last && last->next) {
      last = last->next;
    }

    /* Add to the end of the list */
    if (!last) {
      olsr_cnf->smart_gw_egress_interfaces = in;
    } else {
      last->next = in;
    }
    free(yyvsp[0]);
  }
}
#line 2965 "src/cfgparser/oparse.c"
    break;

  case 204: /* ssmart_gw_egress_file: TOK_SMART_GW_EGRESS_FILE TOK_STRING  */
#line 1473 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Smart gateway egress file: %s\n", yyvsp[0]->string);
  if (olsr_cnf->smart_gw_egress_file) free(olsr_cnf->smart_gw_egress_file);
  olsr_cnf->smart_gw_egress_file = yyvsp[0]->string;
  free(yyvsp[0]);
}
#line 2976 "src/cfgparser/oparse.c"
    break;

  case 205: /* ismart_gw_egress_file_period: TOK_SMART_GW_EGRESS_FILE_PERIOD TOK_INTEGER  */
#line 1482 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Smart gateway egress file period: %d\n", yyvsp[0]->integer);
  olsr_cnf->smart_gw_egress_file_period = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 2986 "src/cfgparser/oparse.c"
    break;

  case 206: /* ssmart_gw_status_file: TOK_SMART_GW_STATUS_FILE TOK_STRING  */
#line 1490 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Smart gateway status file: %s\n", yyvsp[0]->string);
  if (olsr_cnf->smart_gw_status_file) free(olsr_cnf->smart_gw_status_file);
  olsr_cnf->smart_gw_status_file = yyvsp[0]->string;
  free(yyvsp[0]);
}
#line 2997 "src/cfgparser/oparse.c"
    break;

  case 207: /* ismart_gw_offset_tables: TOK_SMART_GW_OFFSET_TABLES TOK_INTEGER  */
#line 1499 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Smart gateway tables offset: %d\n", yyvsp[0]->integer);
  olsr_cnf->smart_gw_offset_tables = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 3007 "src/cfgparser/oparse.c"
    break;

  case 208: /* ismart_gw_offset_rules: TOK_SMART_GW_OFFSET_RULES TOK_INTEGER  */
#line 1507 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Smart gateway rules offset: %d\n", yyvsp[0]->integer);
  olsr_cnf->smart_gw_offset_rules = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 3017 "src/cfgparser/oparse.c"
    break;

  case 209: /* bsmart_gw_allow_nat: TOK_SMART_GW_ALLOW_NAT TOK_BOOLEAN  */
#line 1515 "src/cfgparser/oparse.y"
{
	PARSER_DEBUG_PRINTF("Smart gateway allow client nat: %s\n", yyvsp[0]->boolean ? "yes" : "no");
	olsr_cnf->smart_gw_allow_nat = yyvsp[0]->boolean;
	free(yyvsp[0]);
}
#line 3027 "src/cfgparser/oparse.c"
    break;

  case 210: /* ismart_gw_period: TOK_SMART_GW_PERIOD TOK_INTEGER  */
#line 1523 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Smart gateway period: %d\n", yyvsp[0]->integer);
  olsr_cnf->smart_gw_period = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 3037 "src/cfgparser/oparse.c"
    break;

  case 211: /* asmart_gw_stablecount: TOK_SMART_GW_STABLECOUNT TOK_INTEGER  */
#line 1531 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Smart gateway stablecount: %d\n", yyvsp[0]->integer);
  olsr_cnf->smart_gw_stablecount = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 3047 "src/cfgparser/oparse.c"
    break;

  case 212: /* asmart_gw_thresh: TOK_SMART_GW_THRESH TOK_INTEGER  */
#line 1539 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Smart gateway threshold: %d\n", yyvsp[0]->integer);
  olsr_cnf->smart_gw_thresh = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 3057 "src/cfgparser/oparse.c"
    break;

  case 213: /* asmart_gw_weight_exitlink_up: TOK_SMART_GW_WEIGHT_EXITLINK_UP TOK_INTEGER  */
#line 1547 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Smart gateway exitlink uplink weight: %d\n", yyvsp[0]->integer);
  olsr_cnf->smart_gw_weight_exitlink_up = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 3067 "src/cfgparser/oparse.c"
    break;

  case 214: /* asmart_gw_weight_exitlink_down: TOK_SMART_GW_WEIGHT_EXITLINK_DOWN TOK_INTEGER  */
#line 1555 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Smart gateway exitlink downlink weight: %d\n", yyvsp[0]->integer);
  olsr_cnf->smart_gw_weight_exitlink_down = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 3077 "src/cfgparser/oparse.c"
    break;

  case 215: /* asmart_gw_weight_etx: TOK_SMART_GW_WEIGHT_ETX TOK_INTEGER  */
#line 1563 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Smart gateway ETX weight: %d\n", yyvsp[0]->integer);
  olsr_cnf->smart_gw_weight_etx = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 3087 "src/cfgparser/oparse.c"
    break;

  case 216: /* asmart_gw_divider_etx: TOK_SMART_GW_DIVIDER_ETX TOK_INTEGER  */
#line 1571 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Smart gateway ETX divider: %d\n", yyvsp[0]->integer);
  olsr_cnf->smart_gw_divider_etx = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 3097 "src/cfgparser/oparse.c"
    break;

  case 217: /* asmart_gw_divider_etx: TOK_SMART_GW_MAX_COST_MAX_ETX TOK_INTEGER  */
#line 1579 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Smart gateway max cost max ETX: %d\n", yyvsp[0]->integer);
  olsr_cnf->smart_gw_path_max_cost_etx_max = yyvsp[0]->integer;
  free(yyvsp[0]);
}
#line 3107 "src/cfgparser/oparse.c"
    break;

  case 218: /* ssmart_gw_uplink: TOK_SMART_GW_UPLINK TOK_STRING  */
#line 1587 "src/cfgparser/oparse.y"
{
	PARSER_DEBUG_PRINTF("Smart gateway uplink: %s\n", yyvsp[0]->string);
	if (strcasecmp(yyvsp[0]->string, GW_UPLINK_TXT[GW_UPLINK_NONE]) == 0) {
		olsr_cnf->smart_gw_type = GW_UPLINK_NONE;
	}
	else if (strcasecmp(yyvsp[0]->string, GW_UPLINK_TXT[GW_UPLINK_IPV4]) == 0) {
		olsr_cnf->smart_gw_type = GW_UPLINK_IPV4;
	}
	else if (strcasecmp(yyvsp[0]->string, GW_UPLINK_TXT[GW_UPLINK_IPV6]) == 0) {
		olsr_cnf->smart_gw_type = GW_UPLINK_IPV6;
	}
	else if (strcasecmp(yyvsp[0]->string, GW_UPLINK_TXT[GW_UPLINK_IPV46]) == 0) {
		olsr_cnf->smart_gw_type = GW_UPLINK_IPV46;
	}
	else {
		fprintf(stderr, "Bad gateway uplink type: %s\n", yyvsp[0]->string);
		YYABORT;
	}
	free(yyvsp[0]);
}
#line 3132 "src/cfgparser/oparse.c"
    break;

  case 219: /* ismart_gw_speed: TOK_SMART_GW_SPEED TOK_INTEGER TOK_INTEGER  */
#line 1610 "src/cfgparser/oparse.y"
{
	PARSER_DEBUG_PRINTF("Smart gateway speed: %u uplink/%u downlink kbit/s\n", yyvsp[-1]->integer, yyvsp[0]->integer);
	smartgw_set_uplink(olsr_cnf, yyvsp[-1]->integer);
	smartgw_set_downlink(olsr_cnf, yyvsp[0]->integer);
	free(yyvsp[-1]);
	free(yyvsp[0]);
}
#line 3144 "src/cfgparser/oparse.c"
    break;

  case 220: /* bsmart_gw_uplink_nat: TOK_SMART_GW_UPLINK_NAT TOK_BOOLEAN  */
#line 1620 "src/cfgparser/oparse.y"
{
	PARSER_DEBUG_PRINTF("Smart gateway uplink nat: %s\n", yyvsp[0]->boolean ? "yes" : "no");
	olsr_cnf->smart_gw_uplink_nat = yyvsp[0]->boolean;
	free(yyvsp[0]);
}
#line 3154 "src/cfgparser/oparse.c"
    break;

  case 221: /* ismart_gw_prefix: TOK_SMART_GW_PREFIX TOK_IPV6_ADDR TOK_INTEGER  */
#line 1628 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Smart gateway prefix: %s %u\n", yyvsp[-1]->string, yyvsp[0]->integer);
	if (inet_pton(olsr_cnf->ip_version, yyvsp[-1]->string, &olsr_cnf->smart_gw_prefix.prefix) == 0) {
	  fprintf(stderr, "Bad IP part of gateway prefix: %s\n", yyvsp[-1]->string);
    YYABORT;
  }
	olsr_cnf->smart_gw_prefix.prefix_len = (uint8_t)yyvsp[0]->integer;
	
	free(yyvsp[-1]);
	free(yyvsp[0]);
}
#line 3170 "src/cfgparser/oparse.c"
    break;

  case 222: /* ismart_gw_prefix: TOK_SMART_GW_PREFIX TOK_IPV6_ADDR TOK_SLASH TOK_INTEGER  */
#line 1640 "src/cfgparser/oparse.y"
{
	PARSER_DEBUG_PRINTF("Smart gateway prefix: %s %u\n", yyvsp[-2]->string, yyvsp[0]->integer);
	if (inet_pton(olsr_cnf->ip_version, yyvsp[-2]->string, &olsr_cnf->smart_gw_prefix.prefix) == 0) {
	  fprintf(stderr, "Bad IP part of gateway prefix: %s\n", yyvsp[-2]->string);
    YYABORT;
  }
	olsr_cnf->smart_gw_prefix.prefix_len = (uint8_t)yyvsp[0]->integer;
	
	free(yyvsp[-2]);
	free(yyvsp[0]);
}
#line 3186 "src/cfgparser/oparse.c"
    break;

  case 223: /* bsrc_ip_routes: TOK_SRC_IP_ROUTES TOK_BOOLEAN  */
#line 1654 "src/cfgparser/oparse.y"
{
	PARSER_DEBUG_PRINTF("Use originator for routes src-ip: %s\n", yyvsp[0]->boolean ? "yes" : "no");
	if (olsr_cnf->ip_version != AF_INET) {
          fprintf(stderr, "Source ip routes not possible with IPV6\n");
          YYABORT;
	}
	else olsr_cnf->use_src_ip_routes = yyvsp[0]->boolean;
	free(yyvsp[0]);
}
#line 3200 "src/cfgparser/oparse.c"
    break;

  case 224: /* amain_ip: TOK_MAIN_IP TOK_IPV4_ADDR  */
#line 1666 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Fixed Main IP: %s\n", yyvsp[0]->string);
  
  if (olsr_cnf->ip_version != AF_INET
      || inet_pton(olsr_cnf->ip_version, yyvsp[0]->string, &olsr_cnf->main_addr) != 1) {
    fprintf(stderr, "Bad main IP: %s\n", yyvsp[0]->string);
    YYABORT;
  }
  else olsr_cnf->unicast_src_ip = olsr_cnf->main_addr;
  free(yyvsp[0]);
}
#line 3216 "src/cfgparser/oparse.c"
    break;

  case 225: /* amain_ip: TOK_MAIN_IP TOK_IPV6_ADDR  */
#line 1678 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Fixed Main IP: %s\n", yyvsp[0]->string);
  
  if (olsr_cnf->ip_version != AF_INET6
      || inet_pton(olsr_cnf->ip_version, yyvsp[0]->string, &olsr_cnf->main_addr) != 1) {
    fprintf(stderr, "Bad main IP: %s\n", yyvsp[0]->string);
    YYABORT;
  }
  free(yyvsp[0]);
}
#line 3231 "src/cfgparser/oparse.c"
    break;

  case 226: /* bset_ipforward: TOK_SET_IPFORWARD TOK_BOOLEAN  */
#line 1691 "src/cfgparser/oparse.y"
{
  PARSER_DEBUG_PRINTF("Set IP-Forward procfile variable: %s\n", yyvsp[0]->boolean ? "yes" : "no");
  olsr_cnf->set_ip_forward = yyvsp[0]->boolean;
  free(yyvsp[0]);
}
#line 3241 "src/cfgparser/oparse.c"
    break;

  case 227: /* plblock: TOK_PLUGIN TOK_STRING  */
#line 1700 "src/cfgparser/oparse.y"
{
  struct plugin_entry *pe, *last;
  
  pe = olsr_cnf->plugins;
  last = NULL;
  while (pe != NULL) {
    if (strcmp(pe->name, yyvsp[0]->string) == 0) {
      free (yyvsp[0]->string);
      break;
    }
    last = pe;
    pe = pe->next;
  }

  if (pe != NULL) {
    /* remove old plugin from list to add it later at the beginning */
    if (last) {
      last->next = pe->next;
    }
    else {
      olsr_cnf->plugins = pe->next;
    }
  }
  else {
    pe = malloc(sizeof(*pe));

    if (pe == NULL) {
      fprintf(stderr, "Out of memory(ADD PL)\n");
      YYABORT;
    }

    pe->name = yyvsp[0]->string;
    pe->params = NULL;

    PARSER_DEBUG_PRINTF("Plugin: %s\n", yyvsp[0]->string);
  }
  
  /* Queue */
  pe->next = olsr_cnf->plugins;
  olsr_cnf->plugins = pe;

  free(yyvsp[0]);
}
#line 3289 "src/cfgparser/oparse.c"
    break;

  case 228: /* plparam: TOK_PLPARAM TOK_STRING TOK_STRING  */
#line 1746 "src/cfgparser/oparse.y"
{
  struct plugin_param *pp = malloc(sizeof(*pp));
  char *p;
  
  if (pp == NULL) {
    fprintf(stderr, "Out of memory(ADD PP)\n");
    YYABORT;
  }
  
  PARSER_DEBUG_PRINTF("Plugin param key:\"%s\" val: \"%s\"\n", yyvsp[-1]->string, yyvsp[0]->string);
  
  pp->key = yyvsp[-1]->string;
  pp->value = yyvsp[0]->string;

  /* Lower-case the key */
  for (p = pp->key; *p; p++) {
    *p = tolower(*p);
  }

  /* Queue */
  pp->next = olsr_cnf->plugins->params;
  olsr_cnf->plugins->params = pp;

  free(yyvsp[-1]);
  free(yyvsp[0]);
}
#line 3320 "src/cfgparser/oparse.c"
    break;

  case 229: /* vcomment: TOK_COMMENT  */
#line 1775 "src/cfgparser/oparse.y"
{
    //PARSER_DEBUG_PRINTF("Comment\n");
}
#line 3328 "src/cfgparser/oparse.c"
    break;


#line 3332 "src/cfgparser/oparse.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 1782 "src/cfgparser/oparse.y"


void yyerror (const char *string)
{
  fprintf(stderr, "Config line %d: %s\n", current_line, string);
}
