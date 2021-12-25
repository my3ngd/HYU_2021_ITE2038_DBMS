-- Print the Pokémon's name after the Pokémon with the name Charmander has evolved twice.

SELECT  P.name
FROM    Pokemon AS P
WHERE   P.id = (
    SELECT  E.after_id
    FROM    Evolution AS E
    WHERE   E.before_id = (
        SELECT  Ev.after_id
        FROM    Evolution AS Ev, Pokemon AS pk
        WHERE   Ev.before_id = pk.id
            AND pk.name = 'Charmander'
    )
);
