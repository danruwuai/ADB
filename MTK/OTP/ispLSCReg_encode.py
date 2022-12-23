import sys
# Settings
m = 15
n = 15
width = int(sys.argv[1])
height = int(sys.argv[2])
#width = 3264
#height = 2448
print('resolution:'+ str(width) +'x' + str(height))

# Calculation
XNUM = m-2
YNUM = n-2
X_BLK_SIZE = int((width/2) / (m-1))
Y_BLK_SIZE = int((height/2) / (n-1))
X_LAST_BLK_SIZE = (width/2) - X_BLK_SIZE*XNUM
Y_LAST_BLK_SIZE = (height/2) - Y_BLK_SIZE*YNUM

A = '{0:x}'.format(int(XNUM))
BBB = '0'*(3-len('{0:x}'.format(int(X_BLK_SIZE))))+'{0:x}'.format(int(X_BLK_SIZE))
C = '{0:x}'.format(int(YNUM))
DDD = '0'*(3-len('{0:x}'.format(int(Y_BLK_SIZE)))) + '{0:x}'.format(int(Y_BLK_SIZE))
EEEE = '0'*(4-len('{0:x}'.format(int(X_LAST_BLK_SIZE)))) + '{0:x}'.format(int(X_LAST_BLK_SIZE))
FFFF = '0'*(4-len('{0:x}'.format(int(Y_LAST_BLK_SIZE)))) + '{0:x}'.format(int(Y_LAST_BLK_SIZE))

lscLSCReg = '{0,' + '0x' + A + BBB + C +DDD + ',0,'+'0x'+EEEE+FFFF+',0}'

#lscLSCReg = BBB

print(lscLSCReg)

# Dump lscLSCReg

outFile = 'lspLSCReg_encode'+'_'+str(width)+'_'+str(height)+'.txt'
fout=open(outFile,"w+")
fout.write(lscLSCReg)
fout.close()