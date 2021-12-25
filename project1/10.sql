-- Print the names of Pokémon that have not been caught by any trainer in alphabetical order

SELECT  P.name
FROM    Pokemon AS P
WHERE   P.id NOT IN (
    SELECT  CP.pid
    FROM    CatchedPokemon AS CP
    )
ORDER BY P.name ASC;
