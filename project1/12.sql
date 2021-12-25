-- Print the pre-evolution names of the Pokémon
-- whose ids decrease as they evolve in alphabetical order.

SELECT  P.name
FROM    Pokemon AS P, Evolution AS E
WHERE   E.after_id < E.before_id
    AND P.id = E.before_id
ORDER BY P.name ASC;
