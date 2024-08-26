import math

noscs = 128
inArray = [False]*(noscs+1)
primes = [ 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131 ]
partialsLeft = []
partialsRight = []
numberLeft = True
blockLeft = False

for p in reversed(primes):
  print("<tr><td>",p,"</td><td>",end="")
  if blockLeft:
    numberLeft = True
  else:
    numberLeft = False
  for i in range(1,noscs+1):
    if i*p > noscs:
      break
    if not inArray[i*p]:
      if numberLeft:
        partialsLeft.append(i*p)
        print(i*p, end=" ")
        numberLeft = False
      else:
        partialsRight.append(i*p)
        print('<font color="ff8080">',i*p,'</font>', sep="",end=" ")
        numberLeft = True
      inArray[i*p] = True
  print("</td></tr>")
  blockLeft = not blockLeft

print("\nleft:",len(partialsLeft)," right:",len(partialsRight),"\n")

print("    ", end="")
for i in range(2,noscs+1):
  if i in partialsRight:
    print("true", end=", ")
  else:
    print("false", end=", ")
  if (i%10 == 9):
    print("\n    ", end="")
print("")
