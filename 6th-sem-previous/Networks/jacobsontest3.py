roll = input("enter roll no:").lower()
# 15yZ12345
alpha = 0
for char in roll:
    try:
        val = int(char)
    except:
        val = ord(char) - 96
    alpha += val

alpha = alpha % 5
alpha = alpha / 10.0
alpha = alpha + 0.5
print("alpha", alpha)

beta = alpha / 2.0

R1 = 1100
R2 = 1500

SRTT1 = (1-alpha)*R1
RTTVAR1 = (1-beta)*abs(SRTT1 - R1)
print("SRTT1", SRTT1)
print("RTTVAR1", RTTVAR1)

SRTT2 = alpha*SRTT1 +  (1-alpha)*R2
RTTVAR2 = beta*RTTVAR1 + (1-beta)*abs(SRTT2 - R2)

print("SRTT2", SRTT2)
print("RTTVAR2", RTTVAR2)

RTO = SRTT2 + (4 * RTTVAR2)
print("RTO: ", RTO)
if RTO < 1000:
    print("BUT RTO WILL BE 1 SEC")