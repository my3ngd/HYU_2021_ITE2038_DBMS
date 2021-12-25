-- Print out the names of all trainers from Sangnok city
-- who caught a Pokémon whose name starts with P in alphabetical order

SELECT  DISTINCT T.name
FROM    Trainer AS T, CatchedPokemon AS CP, Pokemon AS P
WHERE   T.hometown = 'Sangnok city'
    AND CP.owner_id = T.id
    AND P.id = CP.pid
    AND P.name LIKE 'P%'
ORDER BY T.name;
