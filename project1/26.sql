-- Print the name of the trainer with the highest total of the Pokémon levels caught
-- and the total number of levels.

SELECT  T_S.tname, T_S.s
FROM    (
    SELECT  t.name AS tname, SUM(CP.level) AS s
    FROM    Trainer AS t, CatchedPokemon AS CP
    WHERE   t.id = CP.owner_id
    GROUP BY tname
    ) AS T_S
WHERE   T_S.s >= ALL(
    SELECT  SUM(CP.level)
    FROM    Trainer AS t, CatchedPokemon AS CP
    WHERE   t.id = CP.owner_id
    GROUP BY t.name
    );
