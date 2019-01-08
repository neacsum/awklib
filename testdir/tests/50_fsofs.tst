BEGIN {FS=":" ; OFS=":"}
{print NF " ",$0}

##Input
1root:EMpNB8Zp56:0:0:Super-User,,,,,,,:/:/bin/sh
2roottcsh:*:0:0:Super-User running tcsh [cbm]:/:/bin/tcsh
3sysadm:*:0:0:System V Administration:/usr/admin:/bin/sh
4diag:*:0:996:Hardware Diagnostics:/usr/diags:/bin/csh
5daemon:*:1:1:daemons:/:/bin/sh
6bin:*:2:2:System Tools Owner:/bin:/dev/null
7nuucp:BJnuQbAo:6:10:UUCP.Admin:/usr/spool/uucppublic:/usr/lib/uucp/uucico
8uucp:*:3:5:UUCP.Admin:/usr/lib/uucp:
9sys:*:4:0:System Activity Owner:/usr/adm:/bin/sh
10adm:*:5:3:Accounting Files Owner:/usr/adm:/bin/sh
##Output
7 :1root:EMpNB8Zp56:0:0:Super-User,,,,,,,:/:/bin/sh
7 :2roottcsh:*:0:0:Super-User running tcsh [cbm]:/:/bin/tcsh
7 :3sysadm:*:0:0:System V Administration:/usr/admin:/bin/sh
7 :4diag:*:0:996:Hardware Diagnostics:/usr/diags:/bin/csh
7 :5daemon:*:1:1:daemons:/:/bin/sh
7 :6bin:*:2:2:System Tools Owner:/bin:/dev/null
7 :7nuucp:BJnuQbAo:6:10:UUCP.Admin:/usr/spool/uucppublic:/usr/lib/uucp/uucico
7 :8uucp:*:3:5:UUCP.Admin:/usr/lib/uucp:
7 :9sys:*:4:0:System Activity Owner:/usr/adm:/bin/sh
7 :10adm:*:5:3:Accounting Files Owner:/usr/adm:/bin/sh
##End
