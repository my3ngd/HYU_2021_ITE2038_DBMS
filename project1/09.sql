-- Print the name of the trainer who caught 4 or more Pokémon
-- and the nickname of the Pokémon with the highest level among the Pokémon
-- caught by that trainer in alphabetical order of the Pokémon nicknames.
-- (If there is more than one highest level, just print all cases.)

SELECT  T.name, CP.nickname
FROM    Trainer AS T, CatchedPokemon AS CP
WHERE   T.id = CP.owner_id
    AND T.name IN (
        SELECT  Tr.name
        FROM    Trainer AS Tr, CatchedPokemon AS Cp
        WHERE   Tr.id = Cp.owner_id
        GROUP BY Tr.name
        HAVING COUNT(*) >= 4
    )
    AND CP.level = ANY(
        SELECT  MAX(Cp.level)
        FROM    CatchedPokemon AS Cp
        WHERE   T.id = Cp.owner_id
        GROUP BY T.id
        HAVING COUNT(*) >= 4
    )
ORDER BY CP.nickname ASC;
