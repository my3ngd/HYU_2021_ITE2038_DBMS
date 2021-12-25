-- Print the names of the trainers who caught two or more of the same Pokémon in alphabetical order.

SELECT  DISTINCT T.name
FROM    Trainer AS T,
        CatchedPokemon AS CP,
        (
            SELECT  t1.id AS Tid, P.id AS Pid, CP.id AS Cid
            FROM    Trainer AS t1, CatchedPokemon AS CP, Pokemon AS P
            WHERE   t1.id = CP.owner_id
                AND P.id = CP.pid
        ) AS T1, (
            SELECT  t2.id AS Tid, P.id AS Pid, CP.id AS Cid
            FROM    Trainer AS t2, CatchedPokemon AS CP, Pokemon AS P
            WHERE   t2.id = CP.owner_id
                AND P.id = CP.pid
        ) AS T2
WHERE   T.id = T1.Tid
    AND T.id = T2.Tid
    AND T.id = CP.owner_id
    AND T1.Cid = CP.id
    AND T1.Tid = T2.Tid
    AND T1.Pid = T2.Pid
    AND T1.Cid <> T2.Cid
ORDER BY T.name ASC;
