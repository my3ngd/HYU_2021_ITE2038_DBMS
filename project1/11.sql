-- Among the Pokémon caught by the gym leader of Sangnok City,
-- print the nicknames of the Pokémon whose type is water in alphabetical order.

SELECT  CP.nickname
FROM    CatchedPokemon AS CP,
        Gym AS G,
        Trainer AS T,
        Pokemon AS P
WHERE   CP.owner_id = T.id
    AND T.id = G.leader_id
    AND G.city = 'Sangnok City'
    AND CP.pid = P.id
    AND P.type = 'water'
ORDER BY CP.nickname ASC;
