-- Print the trainer's name and the number of Pokémon
-- caught by that trainer in alphabetical order by the trainer's name.

SELECT  T.name, COUNT(*)
FROM    Trainer AS T, CatchedPokemon AS CP
WHERE   T.id = CP.owner_id
GROUP BY T.id
ORDER BY T.name ASC;
