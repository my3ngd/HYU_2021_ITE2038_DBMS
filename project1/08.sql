-- Print the number of Pokémon caught by each type in alphabetical order by type name.

SELECT  COUNT(*)
FROM    Pokemon AS P, CatchedPokemon AS CP
WHERE   P.id = CP.pid
GROUP BY P.type
ORDER BY P.type ASC;
