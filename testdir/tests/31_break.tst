{
for (i=1; i <= NF; i++)
	if ($i ~ /^[a-z|A-Z]+$/) {
		print $i " is alphabetic"
		break
	}
}
##Input
1r oot EMpNB8Zp56 0 0 Super-User,,,,,,, / /bin/sh
2r oottcsh * 0 0 Super-User running tcsh [cbm] / /bin/tcsh
3s ysadm * 0 0 System V Administration /usr/admin /bin/sh
4d iag * 0 996 Hardware Diagnostics /usr/diags /bin/csh
5d aemon * 1 1 daemons / /bin/sh
6b in * 2 2 System Tools Owner /bin /dev/null
7n uucp BJnuQbAo 6 10 UUCP.Admin /usr/spool/uucppublic /usr/lib/uucp/uucico
8u ucp * 3 5 UUCP.Admin /usr/lib/uucp 
9s ys * 4 0 System Activity Owner /usr/adm /bin/sh
10 adm * 5 3 Accounting Files Owner /usr/adm /bin/sh
11 lp * 9 9 Print Spooler Owner /var/spool/lp /bin/sh
12 auditor * 11 0 Audit Activity Owner /auditor /bin/sh
13 dbadmin * 12 0 Security Database Owner /dbadmin /bin/sh
14 bootes dcon 50 1 Tom Killian (DO NOT REMOVE) /tmp 
15 cdjuke dcon 51 1 Tom Killian (DO NOT REMOVE) /tmp 
16 rfindd * 66 1 Rfind Daemon and Fsdump /var/rfindd /bin/sh
17 EZsetup * 992 998 System Setup /var/sysadmdesktop/EZsetup /bin/csh
18 demos * 993 997 Demonstration User /usr/demos /bin/csh
19 tutor * 994 997 Tutorial User /usr/tutor /bin/csh
20 tour * 995 997 IRIS Space Tour /usr/people/tour /bin/csh
21 guest nfP4/Wpvio/Rw 998 998 Guest Account /usr/people/guest /bin/csh
22 4Dgifts 0nWRTZsOMt. 999 998 4Dgifts Account /usr/people/4Dgifts /bin/csh
23 nobody * 60001 60001 SVR4 nobody uid /dev/null /dev/null
24 noaccess * 60002 60002 uid no access /dev/null /dev/null
##Output
oot is alphabetic
oottcsh is alphabetic
ysadm is alphabetic
iag is alphabetic
aemon is alphabetic
in is alphabetic
uucp is alphabetic
ucp is alphabetic
ys is alphabetic
adm is alphabetic
lp is alphabetic
auditor is alphabetic
dbadmin is alphabetic
bootes is alphabetic
cdjuke is alphabetic
rfindd is alphabetic
EZsetup is alphabetic
demos is alphabetic
tutor is alphabetic
tour is alphabetic
guest is alphabetic
Account is alphabetic
nobody is alphabetic
noaccess is alphabetic
##END

