var('a b q0 q1 q2')
P(x) = x*(x-1/2)
Q(x) = q0+q1*x+q2*x^2
P1(x) = diff(P(x),x)
Q1(x) = diff(Q(x),x)
S = solve([P(a)==Q(a), P1(a)*Q(a)==P(a)*Q1(a), P(b)==sqrt(2)/2*Q(b)], q0,q1,q2)
print(S)
[print((P(x)/Q(x)).substitute(s)) for s in S] 

