import {native} from "../native";
import {BaseObject} from "../object";
import {DataFormat} from "../data_format";
import * as Collection from "../core/collection";
import {Certificate} from "./cert";

const DEFAULT_DATA_FORMAT = DataFormat.DER;

/**
 * Представление коллекции `X509` сертификатов
 */
export class CertificateCollection extends BaseObject implements Collection.ICollectionWrite {

    constructor() {
        super();

        this.handle = new native.PKI.CertificateCollection();
    }

    /**
     * Возвращает элемент из коллекции по заданному индексу
     * @param index Индекс элемента в коллекции
     */
    items(index: number): Certificate {
        return  <Certificate> Certificate.nativeCreate(this.handle.items(index));
    }

    /**
     * Возвращает размер коллекции
     * @param index Индекс элемента в коллекции
     */
    get length(): number {
        return this.handle.length();
    }

    /**
     * Добавляет новый элемент в коллекцию
     * @param cert Элемент для добавления в коллекцию
     */
    push(cert: Certificate): void {
        this.handle.push(cert.handle);
    }

    /**
     * Удаляет последний элемент их коллекции
     */
    pop(): void {
        this.handle.pop();
    }

    /**
     * Удаляет элемент из коллекции по заданному индексу
     * @param index Индекс элемента в коллекции
     */
    removeAt(index: number): void {
        this.handle.removeAt(index);
    }

}