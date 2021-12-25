-- Print the names of Pokémon whose names start with an alphabetic vowel in alphabetical order.

SELECT  P.name
FROM    Pokemon AS P
WHERE   P.name LIKE 'A%' OR P.name LIKE 'a%'
    OR  P.name LIKE 'E%' OR P.name LIKE 'e%'
    OR  P.name LIKE 'I%' OR P.name LIKE 'i%'
    OR  P.name LIKE 'O%' OR P.name LIKE 'o%'
    OR  P.name LIKE 'U%' OR P.name LIKE 'u%'
ORDER BY P.name ASC;
