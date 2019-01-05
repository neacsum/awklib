#change field separator character
BEGIN	{FS=":"}
	{print $1, $2, $3}

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
11lp:*:9:9:Print Spooler Owner:/var/spool/lp:/bin/sh
12auditor:*:11:0:Audit Activity Owner:/auditor:/bin/sh
13dbadmin:*:12:0:Security Database Owner:/dbadmin:/bin/sh
14bootes:dcon:50:1:Tom Killian (DO NOT REMOVE):/tmp:
15cdjuke:dcon:51:1:Tom Killian (DO NOT REMOVE):/tmp:
16rfindd:*:66:1:Rfind Daemon and Fsdump:/var/rfindd:/bin/sh
17EZsetup:*:992:998:System Setup:/var/sysadmdesktop/EZsetup:/bin/csh
18demos:*:993:997:Demonstration User:/usr/demos:/bin/csh
19tutor:*:994:997:Tutorial User:/usr/tutor:/bin/csh
20tour:*:995:997:IRIS Space Tour:/usr/people/tour:/bin/csh
21guest:nfP4/Wpvio/Rw:998:998:Guest Account:/usr/people/guest:/bin/csh
224Dgifts:0nWRTZsOMt.:999:998:4Dgifts Account:/usr/people/4Dgifts:/bin/csh
23nobody:*:60001:60001:SVR4 nobody uid:/dev/null:/dev/null
24noaccess:*:60002:60002:uid no access:/dev/null:/dev/null
25nobody:*:-2:-2:original nobody uid:/dev/null:/dev/null
26rje:*:8:8:RJE Owner:/usr/spool/rje:
27changes:*:11:11:system change log:/:
28dist:sorry:9999:4:file distributions:/v/adm/dist:/v/bin/sh
29man:*:99:995:On-line Manual Owner:/:
30phoneca:*:991:991:phone call log [tom]:/v/adm/log:/v/bin/sh
##Output
1root EMpNB8Zp56 0
2roottcsh * 0
3sysadm * 0
4diag * 0
5daemon * 1
6bin * 2
7nuucp BJnuQbAo 6
8uucp * 3
9sys * 4
10adm * 5
11lp * 9
12auditor * 11
13dbadmin * 12
14bootes dcon 50
15cdjuke dcon 51
16rfindd * 66
17EZsetup * 992
18demos * 993
19tutor * 994
20tour * 995
21guest nfP4/Wpvio/Rw 998
224Dgifts 0nWRTZsOMt. 999
23nobody * 60001
24noaccess * 60002
25nobody * -2
26rje * 8
27changes * 11
28dist sorry 9999
29man * 99
30phoneca * 991
## END
