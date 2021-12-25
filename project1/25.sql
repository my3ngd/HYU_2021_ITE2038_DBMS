-- Print the names of the evolutionary stage 2 Pokémon in alphabetical order.
-- (ex. In the case of SquirtleBuggy-Turtle King, Buggy, which is the Pokémon with stage 2 evolution,
-- must be printed out, and Pokémon whose stage 2 evolution is the final evolution form also must also be printed.)

SELECT  P.name
FROM    Pokemon AS P, (
    SELECT  E.after_id
    FROM    Evolution AS E
    WHERE   E.before_id NOT IN (
        SELECT  e.after_id
        FROM    Evolution AS e
        )
    ) AS EV
WHERE   EV.after_id = P.id
ORDER BY P.name ASC;
