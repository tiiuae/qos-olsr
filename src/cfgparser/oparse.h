/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_SRC_CFGPARSER_OPARSE_H_TMP_INCLUDED
# define YY_YY_SRC_CFGPARSER_OPARSE_H_TMP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    TOK_SLASH = 258,               /* TOK_SLASH  */
    TOK_OPEN = 259,                /* TOK_OPEN  */
    TOK_CLOSE = 260,               /* TOK_CLOSE  */
    TOK_STRING = 261,              /* TOK_STRING  */
    TOK_INTEGER = 262,             /* TOK_INTEGER  */
    TOK_FLOAT = 263,               /* TOK_FLOAT  */
    TOK_BOOLEAN = 264,             /* TOK_BOOLEAN  */
    TOK_IPV6TYPE = 265,            /* TOK_IPV6TYPE  */
    TOK_DEBUGLEVEL = 266,          /* TOK_DEBUGLEVEL  */
    TOK_IPVERSION = 267,           /* TOK_IPVERSION  */
    TOK_HNA4 = 268,                /* TOK_HNA4  */
    TOK_HNA6 = 269,                /* TOK_HNA6  */
    TOK_PLUGIN = 270,              /* TOK_PLUGIN  */
    TOK_INTERFACE_DEFAULTS = 271,  /* TOK_INTERFACE_DEFAULTS  */
    TOK_INTERFACE = 272,           /* TOK_INTERFACE  */
    TOK_NOINT = 273,               /* TOK_NOINT  */
    TOK_TOS = 274,                 /* TOK_TOS  */
    TOK_OLSRPORT = 275,            /* TOK_OLSRPORT  */
    TOK_RTPROTO = 276,             /* TOK_RTPROTO  */
    TOK_RTTABLE = 277,             /* TOK_RTTABLE  */
    TOK_RTTABLE_DEFAULT = 278,     /* TOK_RTTABLE_DEFAULT  */
    TOK_RTTABLE_TUNNEL = 279,      /* TOK_RTTABLE_TUNNEL  */
    TOK_RTTABLE_PRIORITY = 280,    /* TOK_RTTABLE_PRIORITY  */
    TOK_RTTABLE_DEFAULTOLSR_PRIORITY = 281, /* TOK_RTTABLE_DEFAULTOLSR_PRIORITY  */
    TOK_RTTABLE_TUNNEL_PRIORITY = 282, /* TOK_RTTABLE_TUNNEL_PRIORITY  */
    TOK_RTTABLE_DEFAULT_PRIORITY = 283, /* TOK_RTTABLE_DEFAULT_PRIORITY  */
    TOK_WILLINGNESS = 284,         /* TOK_WILLINGNESS  */
    TOK_IPCCON = 285,              /* TOK_IPCCON  */
    TOK_FIBMETRIC = 286,           /* TOK_FIBMETRIC  */
    TOK_FIBMETRICDEFAULT = 287,    /* TOK_FIBMETRICDEFAULT  */
    TOK_USEHYST = 288,             /* TOK_USEHYST  */
    TOK_HYSTSCALE = 289,           /* TOK_HYSTSCALE  */
    TOK_HYSTUPPER = 290,           /* TOK_HYSTUPPER  */
    TOK_HYSTLOWER = 291,           /* TOK_HYSTLOWER  */
    TOK_POLLRATE = 292,            /* TOK_POLLRATE  */
    TOK_NICCHGSPOLLRT = 293,       /* TOK_NICCHGSPOLLRT  */
    TOK_TCREDUNDANCY = 294,        /* TOK_TCREDUNDANCY  */
    TOK_MPRCOVERAGE = 295,         /* TOK_MPRCOVERAGE  */
    TOK_LQ_LEVEL = 296,            /* TOK_LQ_LEVEL  */
    TOK_LQ_FISH = 297,             /* TOK_LQ_FISH  */
    TOK_LQ_AGING = 298,            /* TOK_LQ_AGING  */
    TOK_LQ_PLUGIN = 299,           /* TOK_LQ_PLUGIN  */
    TOK_LQ_NAT_THRESH = 300,       /* TOK_LQ_NAT_THRESH  */
    TOK_LQ_MULT = 301,             /* TOK_LQ_MULT  */
    TOK_CLEAR_SCREEN = 302,        /* TOK_CLEAR_SCREEN  */
    TOK_PLPARAM = 303,             /* TOK_PLPARAM  */
    TOK_MIN_TC_VTIME = 304,        /* TOK_MIN_TC_VTIME  */
    TOK_LOCK_FILE = 305,           /* TOK_LOCK_FILE  */
    TOK_USE_NIIT = 306,            /* TOK_USE_NIIT  */
    TOK_SMART_GW = 307,            /* TOK_SMART_GW  */
    TOK_SMART_GW_ALWAYS_REMOVE_SERVER_TUNNEL = 308, /* TOK_SMART_GW_ALWAYS_REMOVE_SERVER_TUNNEL  */
    TOK_SMART_GW_USE_COUNT = 309,  /* TOK_SMART_GW_USE_COUNT  */
    TOK_SMART_GW_TAKEDOWN_PERCENTAGE = 310, /* TOK_SMART_GW_TAKEDOWN_PERCENTAGE  */
    TOK_SMART_GW_INSTANCE_ID = 311, /* TOK_SMART_GW_INSTANCE_ID  */
    TOK_SMART_GW_POLICYROUTING_SCRIPT = 312, /* TOK_SMART_GW_POLICYROUTING_SCRIPT  */
    TOK_SMART_GW_EGRESS_IFS = 313, /* TOK_SMART_GW_EGRESS_IFS  */
    TOK_SMART_GW_EGRESS_FILE = 314, /* TOK_SMART_GW_EGRESS_FILE  */
    TOK_SMART_GW_EGRESS_FILE_PERIOD = 315, /* TOK_SMART_GW_EGRESS_FILE_PERIOD  */
    TOK_SMART_GW_STATUS_FILE = 316, /* TOK_SMART_GW_STATUS_FILE  */
    TOK_SMART_GW_OFFSET_TABLES = 317, /* TOK_SMART_GW_OFFSET_TABLES  */
    TOK_SMART_GW_OFFSET_RULES = 318, /* TOK_SMART_GW_OFFSET_RULES  */
    TOK_SMART_GW_ALLOW_NAT = 319,  /* TOK_SMART_GW_ALLOW_NAT  */
    TOK_SMART_GW_PERIOD = 320,     /* TOK_SMART_GW_PERIOD  */
    TOK_SMART_GW_STABLECOUNT = 321, /* TOK_SMART_GW_STABLECOUNT  */
    TOK_SMART_GW_THRESH = 322,     /* TOK_SMART_GW_THRESH  */
    TOK_SMART_GW_WEIGHT_EXITLINK_UP = 323, /* TOK_SMART_GW_WEIGHT_EXITLINK_UP  */
    TOK_SMART_GW_WEIGHT_EXITLINK_DOWN = 324, /* TOK_SMART_GW_WEIGHT_EXITLINK_DOWN  */
    TOK_SMART_GW_WEIGHT_ETX = 325, /* TOK_SMART_GW_WEIGHT_ETX  */
    TOK_SMART_GW_DIVIDER_ETX = 326, /* TOK_SMART_GW_DIVIDER_ETX  */
    TOK_SMART_GW_MAX_COST_MAX_ETX = 327, /* TOK_SMART_GW_MAX_COST_MAX_ETX  */
    TOK_SMART_GW_UPLINK = 328,     /* TOK_SMART_GW_UPLINK  */
    TOK_SMART_GW_UPLINK_NAT = 329, /* TOK_SMART_GW_UPLINK_NAT  */
    TOK_SMART_GW_SPEED = 330,      /* TOK_SMART_GW_SPEED  */
    TOK_SMART_GW_PREFIX = 331,     /* TOK_SMART_GW_PREFIX  */
    TOK_SRC_IP_ROUTES = 332,       /* TOK_SRC_IP_ROUTES  */
    TOK_MAIN_IP = 333,             /* TOK_MAIN_IP  */
    TOK_SET_IPFORWARD = 334,       /* TOK_SET_IPFORWARD  */
    TOK_HOSTLABEL = 335,           /* TOK_HOSTLABEL  */
    TOK_NETLABEL = 336,            /* TOK_NETLABEL  */
    TOK_MAXIPC = 337,              /* TOK_MAXIPC  */
    TOK_IFMODE = 338,              /* TOK_IFMODE  */
    TOK_IPV4MULTICAST = 339,       /* TOK_IPV4MULTICAST  */
    TOK_IP4BROADCAST = 340,        /* TOK_IP4BROADCAST  */
    TOK_IPV4BROADCAST = 341,       /* TOK_IPV4BROADCAST  */
    TOK_IPV6MULTICAST = 342,       /* TOK_IPV6MULTICAST  */
    TOK_IPV4SRC = 343,             /* TOK_IPV4SRC  */
    TOK_IPV6SRC = 344,             /* TOK_IPV6SRC  */
    TOK_IFWEIGHT = 345,            /* TOK_IFWEIGHT  */
    TOK_HELLOINT = 346,            /* TOK_HELLOINT  */
    TOK_HELLOVAL = 347,            /* TOK_HELLOVAL  */
    TOK_TCINT = 348,               /* TOK_TCINT  */
    TOK_TCVAL = 349,               /* TOK_TCVAL  */
    TOK_MIDINT = 350,              /* TOK_MIDINT  */
    TOK_MIDVAL = 351,              /* TOK_MIDVAL  */
    TOK_HNAINT = 352,              /* TOK_HNAINT  */
    TOK_HNAVAL = 353,              /* TOK_HNAVAL  */
    TOK_AUTODETCHG = 354,          /* TOK_AUTODETCHG  */
    TOK_IPV4_ADDR = 355,           /* TOK_IPV4_ADDR  */
    TOK_IPV6_ADDR = 356,           /* TOK_IPV6_ADDR  */
    TOK_DEFAULT = 357,             /* TOK_DEFAULT  */
    TOK_AUTO = 358,                /* TOK_AUTO  */
    TOK_NONE = 359,                /* TOK_NONE  */
    TOK_COMMENT = 360              /* TOK_COMMENT  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_SRC_CFGPARSER_OPARSE_H_TMP_INCLUDED  */
