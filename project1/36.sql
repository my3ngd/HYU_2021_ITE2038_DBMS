-- Print out the Pokemon whose type is water, which cannot be evolved any more, in alphabetical order.
-- (In the case of a Pokémon that has undergone previous evolution, if it cannot evolve in the future,
-- it is judged as a Pokémon that cannot evolve anymore.
-- ex. In the case of Squirtle-Buggy-Turtle King, Turtle King is a Pokémon that can no longer evolve.)

SELECT  P.name
FROM    Pokemon AS P
WHERE   P.type = "Water"
    AND P.id IN (
        SELECT  E.after_id
        FROM    Evolution AS E
    )
    AND P.id NOT IN (
        SELECT  E.before_id
        FROM    Evolution AS E
    )
ORDER BY P.name ASC;
