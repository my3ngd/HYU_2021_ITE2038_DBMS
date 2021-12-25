-- Print the names of trainers that have caught 3 or more Pokémon
-- in the ascending order of the number of Pokémon caught.

SELECT  T.name
FROM    Trainer AS T, CatchedPokemon AS CP
WHERE   CP.owner_id = T.id
GROUP BY T.name
HAVING COUNT(*) >= 3
ORDER BY COUNT(*) ASC;
