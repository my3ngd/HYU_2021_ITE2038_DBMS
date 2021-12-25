-- Print the names of Pokémon caught in common by trainers from Sangnok city
-- and trainers from Blue city in alphabetical order.

SELECT  P.name
FROM    Pokemon AS P
WHERE   P.id IN (
            SELECT  CP.pid
            FROM    Trainer AS T,
                    CatchedPokemon AS CP
            WHERE   P.id = CP.pid
                AND CP.owner_id = T.id
                AND T.hometown = 'Sangnok City'
        )
    AND P.id IN (
            SELECT  CP.pid
            FROM    Trainer AS T,
                    CatchedPokemon AS CP
            WHERE   P.id = CP.pid
                AND CP.owner_id = T.id
                AND T.hometown = 'Blue City'
        )
ORDER BY P.name ASC;
