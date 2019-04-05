#Word frequency counting
{
  for(i=1; i<=NF; i++) {
    if (x[$i] == "")
      y[n++] = $i
    x[$i]++
 }
}
END {
  for (i=0; i<n; i++)
    print (y[i], x[y[i]])
}
##Input
Li Europan lingues es membres del sam familie. Lor separat existentie es un myth.
Por scientie, musica, sport etc, litot Europa usa li sam vocabular.
Li lingues differe solmen in li grammatica, li pronunciation e li plu commun vocabules.
Omnicos directe al desirabilite de un nov lingua franca: On refusa continuar payar custosi traductores.
At solmen va esser necessi far uniform grammatica, pronunciation e plu commun paroles.
Ma quande lingues coalesce, li grammatica del resultant lingue es plu simplic e regulari
quam ti del coalescent lingues. Li nov lingua franca va esser plu simplic e regulari quam
li existent Europan lingues. It va esser tam simplic quam Occidental in fact,
it va esser Occidental. A un Angleso it va semblar un simplificat Angles,
quam un skeptic Cambridge amico dit me que Occidental es.
##Output
Li 3
Europan 2
lingues 3
es 3
membres 1
del 3
sam 2
familie. 1
Lor 1
separat 1
existentie 1
un 5
myth. 1
Por 1
scientie, 1
musica, 1
sport 1
etc, 1
litot 1
Europa 1
usa 1
li 6
vocabular. 1
differe 1
solmen 2
in 2
grammatica, 2
pronunciation 2
e 4
plu 4
commun 2
vocabules. 1
Omnicos 1
directe 1
al 1
desirabilite 1
de 1
nov 2
lingua 2
franca: 1
On 1
refusa 1
continuar 1
payar 1
custosi 1
traductores. 1
At 1
va 5
esser 4
necessi 1
far 1
uniform 1
paroles. 1
Ma 1
quande 1
coalesce, 1
grammatica 1
resultant 1
lingue 1
simplic 3
regulari 2
quam 4
ti 1
coalescent 1
lingues. 2
franca 1
existent 1
It 1
tam 1
Occidental 2
fact, 1
it 2
Occidental. 1
A 1
Angleso 1
semblar 1
simplificat 1
Angles, 1
skeptic 1
Cambridge 1
amico 1
dit 1
me 1
que 1
es. 1
##END