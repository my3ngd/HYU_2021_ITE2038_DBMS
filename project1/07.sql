-- For each city of origin, print the nickname of the Pokémon
-- with the highest level among the Pokémon
-- caught by trainers in the same city of origin in alphabetical order
-- (If there is no trainer in the city, omit the information of that city.
-- If there is more than one highest level, just print all cases.).

SELECT  CP.nickname
FROM    CatchedPokemon AS CP, City AS C, Trainer AS T
WHERE   CP.owner_id = T.id
    AND T.hometown = C.name
    AND (C.name, CP.level) = ANY(
    SELECT  Ct.name, MAX(cp.level)
    FROM    CatchedPokemon AS cp, Trainer AS Tr, City AS Ct
    WHERE   cp.owner_id = Tr.id
        AND Tr.hometown = Ct.name
        AND Ct.name = C.name
    )
ORDER BY CP.nickname ASC;
