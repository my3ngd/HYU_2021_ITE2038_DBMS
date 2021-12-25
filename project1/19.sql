-- Print the total level of Pokémon caught by Trainer Matis

SELECT  SUM(CP.level)
FROM    CatchedPokemon AS CP, Trainer AS T
WHERE   T.name = 'Matis'
    AND T.id = CP.owner_id;
