-- Print the average level of Pokémon Red caught.

SELECT  AVG(CP.level)
FROM    Trainer AS T, CatchedPokemon AS CP
WHERE   CP.owner_id = T.id
    AND T.name = 'Red';
