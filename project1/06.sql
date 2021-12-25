-- Print the name of the gym leader
-- and the average of the Pokémon level
-- caught by each leader in alphabetical order of the leader's name.

SELECT  T.name, AVG(CP.level)
FROM    Gym AS G,
        Trainer AS T,
        CatchedPokemon AS CP
WHERE   G.leader_id = T.id
    AND T.id = CP.owner_id
GROUP BY T.name
ORDER BY T.name ASC;
