import sys
import os
import re

def getRoVersion(v):
	if v[0]<4:
		return "0"
	elif v[0]<5:
		return "1024"
	elif not(v[0]>=7 and v[1]>=2) and v[0]<=7:
		return "2049"
	elif v[0]<8:
		return "3074"
	else:
		return "4096"

def getMenuVersion(v):
	if v[0]==9:
		if (v[1]==0 or v[1]==1):
			return "11272"
		elif v[1]==2:
			return "12288"
		elif v[1]==3:
			return "13330"
		elif v[1]==4:
			return "14336"
		elif v[1]==5:
			return "15360"
		elif v[1]==6 and v[4]=="K":
			return "6616_kor"
		elif v[1]==6:
			return "16404"
		elif v[1]>=7 and v[4]=="K":
			return "7175_kor"
		elif v[1]==7:
			return "17415"
		elif v[1]==9 and v[4]=="U":
			return "20480_usa"
		elif v[1]>=8:
			return "19456"
	elif v[0]==10:
		if v[1]==0:
			if v[4]=="K":
				return "7175_kor"
			if v[4]=="U":
				return "20480_usa"
			else:
				return "19456"
		elif v[1]==1:
			if v[4]=="K":
				return "8192_kor"
			if v[4]=="U":
				return "21504_usa"
			else:
				return "20480"
		elif v[1]==2:
			if v[4]=="K":
				return "9216_kor"
			if v[4]=="U":
				return "22528_usa"
			else:
				return "21504"
		elif v[1]==3:
			if v[4]=="K":
				return "10240_kor"
			if v[4]=="U":
				return "23552_usa"
			else:
				return "22528"
		elif v[1]==4 or v[1]==5:
			if v[4]=="K":
				return "11266_kor"
			if v[4]=="U":
				return "24578_usa"
			else:
				return "23554"
		elif v[1]>=6:
			if v[4]=="K":
				return "12288_kor"
			if v[4]=="U":
				return "25600_usa"
			else:
				return "24576"
	return "unsupported"

def getMsetVersion(v):
	if v[0] == 9 and v[1] < 6:
		return "8203"
	else:
		return "9221"

def getRegion(v):
	return v[4]

def getFirmVersion(v):
	if v[5]==1:
		return "N3DS"
	else:
		return "POST5"


#format : "X.X.X-XR"
version=sys.argv[1]
p=re.compile("^([N]?)([0-9]+)\.([0-9]+)\.([0-9]+)-([0-9]+)([EUJK])")
r=p.match(version)

if r:
	new3DS=(1 if (r.group(1)=="N") else 0)
	cverMajor=int(r.group(2))
	cverMinor=int(r.group(3))
	cverMicro=int(r.group(4))
	nupVersion=int(r.group(5))
	nupRegion=r.group(6)
	extraparams=""
	extraparams+=" LOADROPBIN=1"
	for arg in sys.argv:
		# if(arg=="--enableloadropbin"):
		# 	extraparams+=" LOADROPBIN=1"
		if(arg=="--enableotherapp"):
			extraparams+=" OTHERAPP=1"
		if(arg=="--enablerecovery"):
			extraparams+=" RECOVERY=1"
	v=(cverMajor, cverMinor, cverMicro, nupVersion, nupRegion, new3DS)
	os.system("make clean")	
	os.system("make REGION="+getRegion(v)+" ROVERSION="+getRoVersion(v)+" MSETVERSION="+getMsetVersion(v)+" FIRMVERSION="+getFirmVersion(v)+" MENUVERSION="+getMenuVersion(v)+extraparams)
else:
	print("invalid version format; learn2read.")
