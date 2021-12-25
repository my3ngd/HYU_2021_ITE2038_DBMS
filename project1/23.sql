-- Print the names of Pokémon with two-digit Pokémon IDs in alphabetical order.

SELECT  P.name
FROM    Pokemon AS P
WHERE   P.id >= 10
    AND P.id <= 99
ORDER BY P.name ASC;
