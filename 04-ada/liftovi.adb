with Ada.Text_IO, Ada.Integer_Text_IO;
use  Ada.Text_IO, Ada.Integer_Text_IO;

with Ada.Numerics.Discrete_Random;

procedure liftovi is
    
    protected type BSEM is
        entry Wait;
        procedure Set;
    private
        Pass : Boolean := True;
    end BSEM;

    protected body BSEM is
        entry Wait when Pass is
        begin
            Pass := False;
        end Wait;
        procedure Set is 
        begin
            Pass := True;
        end Set;
    end BSEM;

    -- tipke unutar lifta
	type lift_tipke_t is array (1 .. 4) of Boolean;

    -- stanje lifta
	type lift_t is record
    	kat	    : Integer := 1;
        smjer	: Integer := 0;
        vrata	: Boolean := False;
        tipke	: lift_tipke_t := (1 .. 4 => False);
	end record;

    -- vrsta poziva na katu
	type pozivi_t is record
		gore	: Boolean := False;
        dole	: Boolean := False;
	end record;

    -- tipke na katovima
	type tipke_t is array (1 .. 4) of pozivi_t;

    -- dretva za lift
    task type lift_dretva (ID : Integer) is
        entry start;
        entry key_pressed(floor : in Integer);
    end lift_dretva;

    -- dretva za covjeka
    task type covjek_dretva;
    type covjek_t is access covjek_dretva;
    
    -- globalni podaci
    lift1 : lift_dretva(ID => 1);
    lift2 : lift_dretva(ID => 2);

    katovi : tipke_t;

    covjek : covjek_t;

    sem : BSEM;

    lift1_vrata : array(1 .. 4) of Boolean := (1 .. 4 => False);
    lift2_vrata : array(1 .. 4) of Boolean := (1 .. 4 => False);

    -- ispis stanja sustava
	procedure Ispis ( lift : in lift_t; tipke : in tipke_t; ID : in Integer ) is
		BR_LN : constant := 3;
		BR_ST : constant := 18;
	
		type Prikaz is array (1 .. BR_LN) of String (1 .. BR_ST);
		Slika : Prikaz := (
			"LIFT : 1  2  3  4 ",
			"LIFT :            ",
			"TIPKE: -  -  -  - ");
	
		--indeksi znaka pojedinog kata
		ind : array (1 .. 4) of Integer := (8, 11, 14, 17) ;
		--lijeva i desna "zagrada" oko broja lifta
		lz1 : Character := '[';
		dz1 : Character := ']';
		lz2 : Character := '[';
		dz2 : Character := ']';

	begin
        sem.Wait;
		if lift.smjer = 0 then
			if lift.vrata then
				lz1 := '['; dz1 := ']';
			else
				lz1 := '('; dz1 := ')';
			end if;
		elsif lift.smjer = 1 then
			lz1 := '('; dz1 := '>';
		elsif lift.smjer = -1 then
			lz1 := '<'; dz1 := ')';
		end if;

		Slika (1) ( ind ( lift.kat) - 1 ) := lz1;
		Slika (1) ( ind ( lift.kat) + 1 ) := dz1;

		for i in 1..4 loop
			if lift.tipke(i) then
				Slika (2) ( ind (i) ) := 'x';
			end if;
			
			if tipke(i).gore and tipke(i).dole then
				Slika (3) ( ind (i) ) := 'X';
			elsif tipke(i).gore then
				Slika (3) ( ind (i) ) := 'A';
			elsif tipke(i).dole then
				Slika (3) ( ind (i) ) := 'V';
			else
				Slika (3) ( ind (i) ) := '-';
			end if;
		end loop;

		New_Line (1);
        Put_Line("Stanje za LIFT " & Integer'Image(ID) );
		for i in 1..BR_LN loop
			Put_Line(Slika(i));
		end loop;
		New_Line (1);

        sem.Set;
	end Ispis;


    -- tijelo dretve lift
    task body lift_dretva is
        lift_stanje : lift_t;

        gore_gore : Boolean;
        gore_dole : Boolean;
        dole_gore : Boolean;
        dole_dole : Boolean;

        nastavi_gore : Boolean;
        nastavi_dole : Boolean;

        begin
            accept start;

            loop
                -- inicijalizacija 

                gore_gore := False;
                gore_dole := False;
                dole_gore := False;
                dole_dole := False;

                nastavi_gore := False;
                nastavi_dole := False;

                if ID = 1 then
                    lift1_vrata(lift_stanje.kat) := lift_stanje.vrata;
                elsif ID = 2 then
                    lift2_vrata(lift_stanje.kat) := lift_stanje.vrata;
                end if;

                -- frekvencija osvjezavanja
                delay 5.0;
                
                -- ispisi stanje sustava
                Ispis(lift_stanje, katovi, ID);

                -- pomicanje lifta u slucaju zatvorenih vrata
                if lift_stanje.vrata = False then
                    if lift_stanje.smjer = 1 and lift_stanje.kat <= 4 then
                        lift_stanje.kat := lift_stanje.kat + 1;
                    elsif lift_stanje.smjer = -1 and lift_stanje.kat >= 1 then
                        lift_stanje.kat := lift_stanje.kat - 1;
                    end if;
                else
                    lift_stanje.vrata := False;
                end if;

                -- pritisak tipke u liftu
                select
                    accept key_pressed(floor : in Integer) do
                        lift_stanje.tipke(floor) := True;
                    end key_pressed;
                    or 
                        delay 0.0;
                end select;

                -- promjena stanja lifta
                -- provjeri sve kombinacije smjera i postojecih zahtjeva  
                for i in Integer range 1 .. 4 loop
                    if i > lift_stanje.kat then
                        if katovi(i).gore = True or lift_stanje.tipke(i) = True then
                            gore_gore := True;
                        end if;

                        if katovi(i).dole = True or lift_stanje.tipke(i) = True then
                            gore_dole := True;
                        end if;
                    elsif i < lift_stanje.kat then
                        if katovi(i).dole = True or lift_stanje.tipke(i) = True then
                            dole_dole := True;
                        end if;

                        if katovi(i).gore = True or lift_stanje.tipke(i) = True then
                            dole_gore := True;
                        end if;
                    end if;
                end loop;

                -- provjeri treba li stati na trenutnom katu
                if lift_stanje.tipke(lift_stanje.kat) = True then
                    nastavi_gore := True;
                    nastavi_dole := True;
                end if;

                 if katovi(lift_stanje.kat).gore = True then
                    nastavi_gore := True;
                end if;               

                if katovi(lift_stanje.kat).dole = True then
                    nastavi_dole := True;
                end if;
                
                sem.Wait;

                -- stani i nastavi putovati prema gore
                if lift_stanje.smjer = 1 and nastavi_gore = True then
                    lift_stanje.vrata := True;

                    katovi(lift_stanje.kat).gore := False;
                    lift_stanje.tipke(lift_stanje.kat) := False;
                
                -- stani i nastavi putovati prema dole
                elsif lift_stanje.smjer = -1 and nastavi_dole = True then
                    lift_stanje.vrata := True;

                    katovi(lift_stanje.kat).dole := False;
                    lift_stanje.tipke(lift_stanje.kat) := False;
                
                -- stani ali kreni putovati prema dole
                elsif lift_stanje.smjer = 1 and nastavi_dole = True and
                    (gore_gore = False and gore_dole = False) then
                    lift_stanje.vrata := True;
                    lift_stanje.smjer := -1;

                    katovi(lift_stanje.kat).dole := False;
                    lift_stanje.tipke(lift_stanje.kat) := False;

                -- stani ali kreni putovati prema gore
                elsif lift_stanje.smjer = -1 and nastavi_gore = True and
                    (dole_dole = False and dole_gore = False) then
                    lift_stanje.vrata := True;
                    lift_stanje.smjer := 1;

                    katovi(lift_stanje.kat).gore := False;
                    lift_stanje.tipke(lift_stanje.kat) := False;

                -- isao sam gore ali zahtjevi su samo dole
                elsif lift_stanje.smjer = 1 and 
                    (gore_gore = False and gore_dole = False) and
                    (dole_dole = True or dole_gore = True) then

                    lift_stanje.smjer := -1;

                -- isao sam dole ali zahtjevi su samo gore
                elsif lift_stanje.smjer = -1 and
                    (gore_gore = True or gore_dole = True) and
                    (dole_dole = False and dole_gore = False) then

                    lift_stanje.smjer := 1;

                -- ako stojis a ima zahtjeva gore -> kreni prema gore
                elsif lift_stanje.smjer = 0 and 
                    (gore_gore = True or gore_dole = True) then

                    lift_stanje.smjer := 1;                   

                -- ako stojis a ima zahtjeva dole -> kreni prema dole
                elsif lift_stanje.smjer = 0 and
                    (dole_dole = True and dole_gore = True) then

                    lift_stanje.smjer := -1;

                -- nema zahtjeva za posluzivanje  
                elsif gore_gore = False and gore_dole = False and
                    dole_gore = False and dole_dole = False then

                    lift_stanje.smjer := 0;

                end if;
                sem.Set;

        end loop;

    end lift_dretva;

    -- tijelo dretve covjeka
    task body covjek_dretva is
        curr_floor : Integer range 1 .. 4;
        dest_floor : Integer range 1 .. 4;

        waiting : Boolean := True;
        lift_id : Integer := 0;

        type Rand_Range is range 1 .. 4;
        package Rand_Int is new Ada.Numerics.Discrete_Random(Rand_Range);

        seed : Rand_Int.Generator;

        begin
            Rand_Int.Reset(seed);
        
            -- stvori novi zahtjev
            curr_floor := Integer(Rand_Int.Random(seed));
            dest_floor := Integer(Rand_Int.Random(seed));

            while (curr_floor = dest_floor) loop
                delay 1.0;
                dest_floor := Integer(Rand_Int.Random(seed));
            end loop;

            Put_Line("Stvaram novi zahtjev: " & Integer'Image(curr_floor) & Integer'Image(dest_floor));

            -- spremi u globalnu strukturu
            sem.Wait;
            if curr_floor < dest_floor then
                katovi(curr_floor).gore := True;
            else
                katovi(curr_floor).dole := True;
            end if;
            sem.Set;

            loop
                delay 1.0;
                -- cekaj da udje u lift
                if waiting = True then
                    -- provjeri ako ima koji lift otvorena vrata na curr_florr
                    -- ako ima -> udji u taj lift, pritisni dest_floor
                    if lift1_vrata(curr_floor) = True then
                        lift_id := 1;
                        waiting := False;
                        lift1.key_pressed(dest_floor);
                        Put_Line("Ulazim u lift 1");
                    elsif lift2_vrata(curr_floor) = True then
                        lift_id := 2;
                        waiting := False;
                        lift2.key_pressed(dest_floor);
                        Put_Line("Ulazim u lift 2");
                    end if;
                else
                    -- provjeri ako je moj lift stigao na odredisni kat
                    -- ako je -> izadji iz lifta i ugasi dretvu
                    if lift_id = 1 and lift1_vrata(dest_floor) = True then
                        Put_Line("Izlazim iz lifta 1");
                        exit;
                    elsif lift_id = 2 and lift2_vrata(dest_floor) = True then
                        Put_Line("Izlazim iz lifta 2");
                        exit;
                    end if;
                end if;

            end loop;
            
    end covjek_dretva;

begin
    lift1.start;
    lift2.start;

    loop
        covjek := new covjek_dretva;
        delay 20.0;
    end loop;

end liftovi;
