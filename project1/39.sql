-- Print the names of Pokémon caught by the gym leader in Rainbow City in alphabetical order.

SELECT  P.name
FROM    Pokemon AS P, Gym AS G, Trainer AS T, CatchedPokemon AS CP
WHERE   G.city = 'Rainbow City'
    AND G.leader_id = T.id
    AND T.id = CP.owner_id
    AND CP.pid = P.id
ORDER BY P.name ASC;
