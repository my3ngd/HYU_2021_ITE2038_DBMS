-- Print the name of the trainer who caught only one type(e.g. water, fire) of Pokémon,
-- the name of the Pokémon, and the number of times caught, in alphabetical order of the trainer name.
SELECT  T.name, P.name, COUNT(P.name)
FROM    Trainer AS T,
        Pokemon AS P,
        CatchedPokemon AS CP,
        (
            SELECT  Tr.name AS name, Tr.id AS id, COUNT(Pk.id), COUNT(DISTINCT Pk.id)
            FROM    Trainer AS Tr, CatchedPokemon AS Cp, Pokemon AS Pk
            WHERE   Tr.id = Cp.owner_id
                AND Cp.pid = Pk.id
            GROUP BY Tr.id
            HAVING COUNT(DISTINCT Pk.type) = 1
        ) AS U
WHERE   U.id = T.id
    AND T.id = CP.owner_id
    AND CP.pid = P.id

GROUP BY T.name, P.name

ORDER BY T.name ASC;
