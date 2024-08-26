var('c q0 q1 q2')
P(x) = x
Q(x) = q0+q1*x+q2*x^2
P1(x) = diff(P(x),x)
Q1(x) = diff(Q(x),x)
S = solve([P(1)==Q(1), P1(0)*P(1)==(P1(1)-Q1(1))*Q(0), 2*P(c)==Q(c)], q0,q1,q2)
print(S)
[print((P(x)/Q(x)).substitute(s)) for s in S] 

